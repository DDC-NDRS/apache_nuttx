/****************************************************************************
 * arch/arm/src/armv7-r/arm_dataabort.c
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

#include <nuttx/irq.h>

#include "sched/sched.h"
#include "arm_internal.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: arm_dataabort
 *
 * Input Parameters:
 *   regs - The standard, ARM register save array.
 *   dfar - Fault address register.
 *   dfsr - Fault status register.
 *
 * Description:
 *   This is the data abort exception handler. The ARM data abort exception
 *   occurs when a memory fault is detected during a data transfer.
 *
 ****************************************************************************/

uint32_t *arm_dataabort(uint32_t *regs, uint32_t dfar, uint32_t dfsr)
{
  struct tcb_s *tcb = this_task();

  tcb->xcp.regs = regs;
  up_set_interrupt_context(true);

  /* Crash -- possibly showing diagnostic debug information. */

  _alert("Data abort. PC: %08" PRIx32 " DFAR: %08" PRIx32 " DFSR: %08"
         PRIx32 "\n", regs[REG_PC], dfar, dfsr);
  PANIC_WITH_REGS("panic", regs);
  return regs; /* To keep the compiler happy */
}
