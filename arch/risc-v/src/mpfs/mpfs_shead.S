/****************************************************************************
 * arch/risc-v/src/mpfs/mpfs_shead.S
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

#include <nuttx/irq.h>

#include "chip.h"
#include "mpfs_memorymap.h"
#include "riscv_internal.h"
#include "riscv_macros.S"

/****************************************************************************
 * Public Symbols
 ****************************************************************************/

  /* Imported symbols */

  .extern __trap_vec

  .section .start, "ax"
#ifdef CONFIG_NUTTSBI
  .global __start_s
#else
  .global __start
#endif

/****************************************************************************
 * Name: __start
 *
 * Description:
 *   Supervisor mode start function.
 *
 * Input Parameters:
 *    a0 - hartid
 *
 ****************************************************************************/

#ifdef CONFIG_NUTTSBI
__start_s:
#else
__start:
#endif
  /* Disable all interrupts in sie */

  csrw CSR_SIE, zero
  csrw CSR_SIP, zero

  /* Set the S-mode trap vector */

  la   t0, __trap_vec
  csrw CSR_STVEC, t0

  /* Clear sscratch */

  csrw CSR_SSCRATCH, zero
  csrw CSR_SCAUSE, zero
  csrw CSR_SEPC, zero

  /* initialize global pointer, global data */

.option push
.option norelax
  la  gp, __global_pointer$
.option pop

  /* Remove MMU mappings (if any) and flush TLB */

  csrw CSR_SATP, zero
  sfence.vma x0, x0

  /* Make sure the writes to CSR stick before continuing */

  fence

  /* Set stack pointer and jump to start */

#ifdef CONFIG_SMP
  addi a1, a0, -1        /* Only cores 1...3 can participate in SMP */
  riscv_set_inital_sp MPFS_IDLESTACK_BASE, SMP_STACK_SIZE, a1
#else
  la   sp, MPFS_IDLESTACK_TOP
#endif

  /* Jump to __mpfs_start with mhartid in a0 */

  tail __mpfs_start
