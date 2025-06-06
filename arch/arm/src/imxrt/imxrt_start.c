/****************************************************************************
 * arch/arm/src/imxrt/imxrt_start.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <assert.h>
#include <debug.h>

#include <nuttx/cache.h>
#include <nuttx/init.h>
#include <arch/barriers.h>
#include <arch/board/board.h>

#include "arm_internal.h"
#include "nvic.h"
#include "ram_vectors.h"

#include "imxrt_clockconfig.h"
#include "imxrt_mpuinit.h"
#include "imxrt_userspace.h"
#include "imxrt_lowputc.h"
#include "imxrt_serial.h"
#include "imxrt_start.h"
#include "imxrt_gpio.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define IDLE_STACK      ((unsigned)&_ebss+CONFIG_IDLETHREAD_STACKSIZE)

#ifdef CONFIG_DEBUG_FEATURES
#define showprogress(c) imxrt_lowputc(c)
#else
#  define showprogress(c)
#endif

/* Memory Map ***************************************************************/

/* 0x2020:0000 - Start of on-chip RAM (OCRAM) and start of .data (_sdata)
 *             - End of .data (_edata) and start of .bss (_sbss)
 *             - End of .bss (_ebss) and bottom of idle stack
 *             - _ebss + CONFIG_IDLETHREAD_STACKSIZE = end of idle stack,
 *               start of heap. NOTE that the ARM uses a decrement before
 *               store stack so that the correct initial value is the end of
 *               the stack + 4;
 * 0x2027:ffff - End of OCRAM and end of heap (assuming 512Kb OCRAM)
 *
 * NOTE:  This assumes that all internal RAM is configured for OCRAM (vs.
 * ITCM or DTCM).  The RAM that holds .data and .bss is called the "Primary
 * RAM".  Many other configurations are possible, including configurations
 * where the primary ram is in external memory.  Those are not considered
 * here.
 */

/****************************************************************************
 * Private Types
 ****************************************************************************/

#ifdef CONFIG_ARMV7M_STACKCHECK
/* we need to get r10 set before we can allow instrumentation calls */

void __start(void) noinstrument_function;
#endif

extern const void * const _vectors[];

/****************************************************************************
 * Name: imxrt_tcmenable
 *
 * Description:
 *   Enable/disable tightly coupled memories.  Size of tightly coupled
 *   memory regions is controlled by GPNVM Bits 7-8.
 *
 ****************************************************************************/

static inline void imxrt_tcmenable(void)
{
  uint32_t regval;

  UP_MB();

  /* Enabled/disabled ITCM */

#ifdef CONFIG_ARMV7M_ITCM
  regval  = NVIC_TCMCR_EN | NVIC_TCMCR_RMW | NVIC_TCMCR_RETEN;
#else
  regval  = getreg32(NVIC_ITCMCR);
  regval &= ~NVIC_TCMCR_EN;
#endif
  putreg32(regval, NVIC_ITCMCR);

  /* Enabled/disabled DTCM */

#ifdef CONFIG_ARMV7M_DTCM
  regval  = NVIC_TCMCR_EN | NVIC_TCMCR_RMW | NVIC_TCMCR_RETEN;
#else
  regval  = getreg32(NVIC_DTCMCR);
  regval &= ~NVIC_TCMCR_EN;
#endif
  putreg32(regval, NVIC_DTCMCR);

  UP_MB();

#ifdef CONFIG_ARMV7M_ITCM
  /* Copy TCM code from flash to ITCM */

#warning Missing logic
#endif
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: __start
 *
 * Description:
 *   This is the reset entry point.
 *
 ****************************************************************************/

osentry_function
void __start(void)
{
  const register uint32_t *src;
  register uint32_t *dest;

  /* Make sure that interrupts are disabled and set MSP */

  __asm__ __volatile__ ("\tcpsid  i\n");
  __asm__ __volatile__ ("MSR MSP, %0\n" : : "r" (IDLE_STACK) :);

  /* Make sure that we use MSP from now on */

  __asm__ __volatile__ ("MSR CONTROL, %0\n" : : "r" (0) :);
  __asm__ __volatile__ ("ISB SY\n");

  /* Make sure VECTAB is set to NuttX vector table
   * and not the one from the boot ROM and have consistency
   * with debugger that automatically set the VECTAB
   */

  putreg32((uint32_t)_vectors, NVIC_VECTAB);

#ifdef CONFIG_ARMV7M_STACKCHECK
  /* Set the stack limit before we attempt to call any functions */

  __asm__ volatile("sub r10, sp, %0" : :
                   "r"(CONFIG_IDLETHREAD_STACKSIZE - 64) :);
#endif

#if defined(CONFIG_BOOT_RUNFROMISRAM) || defined(CONFIG_IMXRT_INIT_FLEXRAM)
    imxrt_ocram_initialize();
#endif

  /* Clear .bss.  We'll do this inline (vs. calling memset) just to be
   * certain that there are no issues with the state of global variables.
   */

  for (dest = (uint32_t *)_sbss; dest < (uint32_t *)_ebss; )
    {
      *dest++ = 0;
    }

  /* Move the initialized data section from his temporary holding spot in
   * FLASH into the correct place in OCRAM.  The correct place in OCRAM is
   * give by _sdata and _edata.  The temporary location is in FLASH at the
   * end of all of the other read-only data (.text, .rodata) at _eronly.
   */

  for (src = (const uint32_t *)_eronly,
       dest = (uint32_t *)_sdata; dest < (uint32_t *)_edata;
      )
    {
      *dest++ = *src++;
    }

  /* Copy any necessary code sections from FLASH to RAM.  The correct
   * destination in OCRAM is given by _sramfuncs and _eramfuncs.  The
   * temporary location is in flash after the data initialization code
   * at _framfuncs.  This should be done before imxrt_clockconfig() is
   * called (in case it has some dependency on initialized C variables).
   */

#ifdef CONFIG_ARCH_RAMFUNCS
  for (src = (const uint32_t *)_framfuncs,
       dest = (uint32_t *)_sramfuncs; dest < (uint32_t *)_eramfuncs;
      )
    {
      *dest++ = *src++;
    }
#endif

#ifdef CONFIG_ARCH_RAMVECTORS
  arm_ramvec_initialize();
#endif

#ifdef CONFIG_ARMV7M_STACKCHECK
  arm_stack_check_init();
#endif

  /* Configure the UART so that we can get debug output as soon as possible */

  imxrt_clockconfig();
  arm_fpuconfig();
  imxrt_lowsetup();
  showprogress('B');

  /* Enable/disable tightly coupled memories */

  imxrt_tcmenable();

  /* Initialize onboard resources */

  imxrt_boardinitialize();

#ifdef CONFIG_ARM_MPU
#ifdef CONFIG_BUILD_PROTECTED
  /* For the case of the separate user-/kernel-space build, perform whatever
   * platform specific initialization of the user memory is required.
   * Normally this just means initializing the user space .data and .bss
   * segments.
   */

  imxrt_userspace();
#endif

  /* Configure the MPU to permit user-space access to its FLASH and RAM (for
   * CONFIG_BUILD_PROTECTED) or to manage cache properties in external
   * memory regions.
   */

  imxrt_mpu_initialize();
#endif

  /* Enable I- and D-Caches */

  up_enable_icache();
  up_enable_dcache();

  /* Perform early serial initialization */

#ifdef USE_EARLYSERIALINIT
  imxrt_earlyserialinit();
#endif

  /* Then start NuttX */

  nx_start();

  /* Shouldn't get here */

  for (; ; );
}
