/****************************************************************************
 * sched/timer/timer_release.c
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

#include <errno.h>

#include <nuttx/irq.h>
#include <nuttx/queue.h>
#include <nuttx/kmalloc.h>

#include "timer/timer.h"

#ifndef CONFIG_DISABLE_POSIX_TIMERS

/****************************************************************************
 * Private Functions
 ****************************************************************************/


/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: timer_release
 *
 * Description:
 *   timer_release implements the heart of timer_delete.  It is private to
 *   the OS internals and differs only in that return value of 1 means that
 *   the timer was not actually deleted.
 *
 * Input Parameters:
 *   timer - The per-thread timer, previously created by the call to
 *     timer_create(), to be deleted.
 *
 * Returned Value:
 *   If the call succeeds, timer_release() will return 0 (OK) or 1 (meaning
 *   that the timer is still valid).  Otherwise, the function will return a
 *   negated errno value:
 *
 *   -EINVAL - The timer specified timerid is not valid.
 *
 ****************************************************************************/

static int timer_disarm_sub(FAR struct posix_timer_s *timer)
{
  /* Some sanity checks */

  if (timer == NULL)
    {
      return -EINVAL;
    }

  /* Release one reference to timer.  Don't delete the timer until the count
   * would decrement to zero.
   */

  if (timer->pt_crefs > 1)
    {
      timer->pt_crefs--;
      return 1;
    }

  /* Cancel the underlying watchdog instance */

  wd_cancel(&timer->pt_wdog);

  /* Cancel any pending notification */

  nxsig_cancel_notification(&timer->pt_work);

  /* Release the timer structure */

  return OK;
}

int timer_disarm(timer_t timerid)
{
  int ret = timer_disarm_sub(timer_gethandle(timerid));
  if (ret < 0)
    {
      set_errno(-ret);
      return ERROR;
    }

  return OK;
}

#endif /* CONFIG_DISABLE_POSIX_TIMERS */