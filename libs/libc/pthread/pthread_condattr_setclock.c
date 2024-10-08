/****************************************************************************
 * libs/libc/pthread/pthread_condattr_setclock.c
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

#include <pthread.h>
#include <errno.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name:  pthread_condattr_setclock
 *
 * Description:
 *   set the clock selection condition variable attribute
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   If successful, the pthread_condattr_setclock() function shall
 *   return zero; otherwise, an error number shall be returned to
 *   indicate the error.
 *
 ****************************************************************************/

int pthread_condattr_setclock(FAR pthread_condattr_t *attr,
                              clockid_t clock_id)
{
  if (!attr ||
      (clock_id != CLOCK_MONOTONIC &&
       clock_id != CLOCK_REALTIME))
    {
      return EINVAL;
    }

  attr->clockid = clock_id;

  return OK;
}
