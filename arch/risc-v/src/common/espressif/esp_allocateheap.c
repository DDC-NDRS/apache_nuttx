/****************************************************************************
 * arch/risc-v/src/common/espressif/esp_allocateheap.c
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

#include <debug.h>
#include <sys/types.h>

#include <arch/board/board.h>
#include <nuttx/arch.h>
#include <nuttx/board.h>
#include <nuttx/mm/mm.h>

#include "riscv_internal.h"
#include "rom/rom_layout.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: up_allocate_heap
 *
 * Description:
 *   This function will be called to dynamically set aside the heap region.
 *
 *   For the kernel build (CONFIG_BUILD_KERNEL=y) with both kernel and
 *   userspace heaps (CONFIG_MM_KERNEL_HEAP=y), this function provides the
 *   size of the unprotected, userspace heap.
 *
 *   If a protected kernel heap is provided, the kernel heap must be
 *   allocated (and protected) by an analogous up_allocate_kheap().
 *
 * Input Parameters:
 *   None.
 *
 * Output Parameters:
 *   heap_start - Address of the beginning of the (initial) memory region.
 *   heap_size  - The size (in bytes) if the (initial) memory region.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

void up_allocate_heap(void **heap_start, size_t *heap_size)
{
  /* These values come from the linker scripts
   * (<chip>_<legacy/mcuboot>_sections.ld and <chip>_flat_memory.ld).
   * Check boards/risc-v/espressif.
   */

  board_autoled_on(LED_HEAPALLOCATE);

  *heap_start = (void *)g_idle_topstack;
  *heap_size  = (uintptr_t)ets_rom_layout_p->dram0_rtos_reserved_start -
                           g_idle_topstack;
}

/****************************************************************************
 * Name: riscv_addregion
 *
 * Description:
 *   RAM may be added in non-contiguous chunks. This routine adds all chunks
 *   that may be used for heap.
 *
 * Input Parameters:
 *   None.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

#if CONFIG_MM_REGIONS > 1
void riscv_addregion(void)
{
}
#endif

