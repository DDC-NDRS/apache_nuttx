/****************************************************************************
 * arch/arm64/src/common/arm64_pgalloc.c
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

#include <string.h>
#include <assert.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/arch.h>
#include <nuttx/addrenv.h>
#include <nuttx/irq.h>
#include <nuttx/pgalloc.h>
#include <nuttx/sched.h>

#include <arch/barriers.h>

#include "sched/sched.h"
#include "addrenv.h"
#include "pgalloc.h"
#include "arm64_mmu.h"

#ifdef CONFIG_BUILD_KERNEL

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pgalloc
 *
 * Description:
 *   If there is a page allocator in the configuration and if and MMU is
 *   available to map physical addresses to virtual address, then function
 *   must be provided by the platform-specific code.  This is part of the
 *   implementation of sbrk().  This function will allocate the requested
 *   number of pages using the page allocator and map them into consecutive
 *   virtual addresses beginning with 'brkaddr'
 *
 *   NOTE:  This function does not use the up_ naming standard because it
 *   is indirectly callable from user-space code via a system trap.
 *   Therefore, it is a system interface and follows a different naming
 *   convention.
 *
 * Input Parameters:
 *   brkaddr - The heap break address.  The next page will be allocated and
 *     mapped to this address.  Must be page aligned.  If the memory manager
 *     has not yet been initialized and this is the first block requested for
 *     the heap, then brkaddr should be zero.  pgalloc will then assigned the
 *     well-known virtual address of the beginning of the heap.
 *   npages - The number of pages to allocate and map.  Mapping of pages
 *     will be contiguous beginning beginning at 'brkaddr'
 *
 * Returned Value:
 *   The (virtual) base address of the mapped page will returned on success.
 *   Normally this will be the same as the 'brkaddr' input. However, if
 *   the 'brkaddr' input was zero, this will be the virtual address of the
 *   beginning of the heap.  Zero is returned on any failure.
 *
 ****************************************************************************/

uintptr_t pgalloc(uintptr_t brkaddr, unsigned int npages)
{
  struct tcb_s          *tcb = this_task();
  struct arch_addrenv_s *addrenv;
  uintptr_t              ptlast;
  uintptr_t              ptlevel;
  uintptr_t              paddr;
  uintptr_t              vaddr;

  DEBUGASSERT(tcb && tcb->addrenv_own);
  addrenv = &tcb->addrenv_own->addrenv;

  /* The current implementation only supports extending the user heap
   * region as part of the implementation of user sbrk().  This function
   * needs to be expanded to also handle (1) extending the user stack
   * space and (2) extending the kernel memory regions as well.
   */

  /* brkaddr = 0 means that no heap has yet been allocated */

  if (!brkaddr)
    {
      brkaddr = addrenv->heapvbase;
    }

  /* Start mapping from the old heap break address */

  vaddr   = brkaddr;
  ptlevel = MMU_PGT_LEVEL_MAX;

  /* Sanity checks */

  DEBUGASSERT(brkaddr >= addrenv->heapvbase);
  DEBUGASSERT(MM_ISALIGNED(brkaddr));

  for (; npages > 0; npages--)
    {
      /* Get the address of the last level page table */

      ptlast = arm64_pgvaddr(arm64_get_pgtable(addrenv, vaddr));
      if (!ptlast)
        {
          return 0;
        }

      /* Allocate physical memory for the new heap */

      paddr = mm_pgalloc(1);
      if (!paddr)
        {
          return 0;
        }

      /* Wipe the memory */

      arm64_pgwipe(paddr);

      /* Then add the reference */

      mmu_ln_setentry(ptlevel, ptlast, paddr, vaddr, MMU_UDATA_FLAGS);
      vaddr += MM_PGSIZE;
    }

  return brkaddr;
}

/****************************************************************************
 * Name: up_allocate_pgheap
 *
 * Description:
 *   If there is a page allocator in the configuration, then this function
 *   must be provided by the platform-specific code.  The OS initialization
 *   logic will call this function early in the initialization sequence to
 *   get the page heap information needed to configure the page allocator.
 *
 ****************************************************************************/

void up_allocate_pgheap(void **heap_start, size_t *heap_size)
{
  DEBUGASSERT(heap_start && heap_size);

  *heap_start = (void *)CONFIG_ARCH_PGPOOL_PBASE;
  *heap_size  = (size_t)CONFIG_ARCH_PGPOOL_SIZE;
}

#endif /* CONFIG_BUILD_KERNEL */
