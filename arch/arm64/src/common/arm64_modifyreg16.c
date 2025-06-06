/****************************************************************************
 * arch/arm64/src/common/arm64_modifyreg16.c
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
#include <debug.h>

#include <nuttx/spinlock.h>

#include "arm64_internal.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static spinlock_t g_modifyreg_lock = SP_UNLOCKED;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: modifyreg16
 *
 * Description:
 *   Atomically modify the specified bits in a memory mapped register
 *
 ****************************************************************************/

void modifyreg16(unsigned int addr, uint16_t clearbits, uint16_t setbits)
{
  irqstate_t flags;
  uint16_t   regval;

  flags   = spin_lock_irqsave(&g_modifyreg_lock);
  regval  = getreg16(addr);
  regval &= ~clearbits;
  regval |= setbits;
  putreg16(regval, addr);
  spin_unlock_irqrestore(&g_modifyreg_lock, flags);
}
