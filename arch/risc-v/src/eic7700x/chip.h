/****************************************************************************
 * arch/risc-v/src/eic7700x/chip.h
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

#ifndef __ARCH_RISCV_SRC_EIC7700X_CHIP_H
#define __ARCH_RISCV_SRC_EIC7700X_CHIP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

/* Include the chip capabilities file */

#include <arch/eic7700x/chip.h>

#include "eic7700x_memorymap.h"

#include "hardware/eic7700x_memorymap.h"
#include "hardware/eic7700x_plic.h"

#include "riscv_internal.h"
#include "riscv_percpu.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Hart ID that booted NuttX (0 to 3) */

#ifndef __ASSEMBLY__
extern int g_eic7700x_boot_hart;
#endif

/****************************************************************************
 * Macro Definitions
 ****************************************************************************/

#ifdef __ASSEMBLY__

/****************************************************************************
 * Name: setintstack
 *
 * Description:
 *   Set the current stack pointer to the "top" of the correct interrupt
 *   stack for the current CPU.
 *
 ****************************************************************************/

#if defined(CONFIG_SMP) && CONFIG_ARCH_INTERRUPTSTACK > 15
.macro  setintstack tmp0, tmp1
  up_cpu_index \tmp0
  li    \tmp1, STACK_ALIGN_DOWN(CONFIG_ARCH_INTERRUPTSTACK)
  mul   \tmp1, \tmp0, \tmp1
  la    \tmp0, g_intstacktop
  sub   sp, \tmp0, \tmp1
.endm
#endif /* CONFIG_SMP && CONFIG_ARCH_INTERRUPTSTACK > 15 */

#if CONFIG_ARCH_INTERRUPTSTACK > 15
#if !defined(CONFIG_SMP) && defined(CONFIG_ARCH_USE_S_MODE)
.macro  setintstack tmp0, tmp1
  csrr    \tmp0, CSR_SCRATCH
  REGLOAD sp, RISCV_PERCPU_IRQSTACK(\tmp0)
.endm
#endif /* !defined(CONFIG_SMP) && defined(CONFIG_ARCH_USE_S_MODE) */
#endif /* CONFIG_ARCH_INTERRUPTSTACK > 15 */

#endif /* __ASSEMBLY__  */
#endif /* __ARCH_RISCV_SRC_EIC7700X_CHIP_H */
