/****************************************************************************
 * libs/libc/wqueue/work_cancel.c
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

#include <assert.h>
#include <errno.h>

#include <nuttx/arch.h>
#include <nuttx/queue.h>
#include <nuttx/wqueue.h>

#include "wqueue/wqueue.h"

#if defined(CONFIG_LIBC_USRWORK) && !defined(__KERNEL__)

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: work_qcancel
 *
 * Description:
 *   Cancel previously queued work.  This removes work from the work queue.
 *   After work has been cancelled, it may be requeued by calling
 *   work_queue() again.
 *
 * Input Parameters:
 *   wqueue - The work queue
 *   work   - The previously queue work structure to cancel
 *
 * Returned Value:
 *   Zero (OK) on success, a negated errno on failure.  This error may be
 *   reported:
 *
 *   -ENOENT - There is no such work queued.
 *   -EINVAL - An invalid work queue was specified
 *
 ****************************************************************************/

static int work_qcancel(FAR struct usr_wqueue_s *wqueue,
                        FAR struct work_s *work)
{
  int ret = -ENOENT;
  int semcount;

  DEBUGASSERT(work != NULL);

  /* Get exclusive access to the work queue */

  while (nxmutex_lock(&wqueue->lock) < 0);

  /* Cancelling the work is simply a matter of removing the work structure
   * from the work queue.  This must be done with interrupts disabled because
   * new work is typically added to the work queue from interrupt handlers.
   */

  if (work->worker != NULL)
    {
      bool is_head = list_is_head(&wqueue->q, &work->node);

      /* Now, remove the work from the work queue */

      list_delete(&work->node);

      if (is_head)
        {
          /* Remove the work at the head of the queue */

          nxsem_get_value(&wqueue->wake, &semcount);
          if (semcount < 1)
            {
              nxsem_post(&wqueue->wake);
            }
        }

      work->worker = NULL;
      ret = OK;
    }

  nxmutex_unlock(&wqueue->lock);
  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: work_cancel
 *
 * Description:
 *   Cancel previously queued user-mode work.  This removes work from the
 *   user mode work queue.  After work has been cancelled, it may be
 *   requeued by calling work_queue() again.
 *
 * Input Parameters:
 *   qid    - The work queue ID (must be USRWORK)
 *   work   - The previously queued work structure to cancel
 *
 * Returned Value:
 *   Zero (OK) on success, a negated errno on failure.  This error may be
 *   reported:
 *
 *   -ENOENT - There is no such work queued.
 *
 ****************************************************************************/

int work_cancel(int qid, FAR struct work_s *work)
{
  if (qid == USRWORK)
    {
      return work_qcancel(&g_usrwork, work);
    }
  else
    {
      return -EINVAL;
    }
}

#endif /* CONFIG_LIBC_USRWORK && !__KERNEL__ */
