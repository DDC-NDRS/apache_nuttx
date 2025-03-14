/****************************************************************************
 * sched/init/nx_bringup.c
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

#include <sched.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <debug.h>

#include <nuttx/arch.h>
#include <nuttx/board.h>
#include <nuttx/fs/fs.h>
#include <nuttx/init.h>
#include <nuttx/macro.h>
#include <nuttx/symtab.h>
#include <nuttx/trace.h>
#include <nuttx/wqueue.h>
#include <nuttx/kthread.h>
#include <nuttx/userspace.h>
#include <nuttx/binfmt/binfmt.h>

#ifdef CONFIG_LEGACY_PAGING
#  include "paging/paging.h"
#endif

#include "sched/sched.h"
#include "wqueue/wqueue.h"
#include "init/init.h"
#include "misc/coredump.h"

#ifdef CONFIG_ETC_ROMFS
#  include <nuttx/drivers/ramdisk.h>
#  include <sys/mount.h>
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration */

#if defined(CONFIG_INIT_NONE)
  /* Kconfig logic will set CONFIG_INIT_NONE if dependencies are not met */

#  error No initialization mechanism selected (CONFIG_INIT_NONE)

#else
#  if !defined(CONFIG_INIT_ENTRY) && !defined(CONFIG_INIT_FILE)
  /* For backward compatibility with older defconfig files when this was
   * the way things were done.
   */

#    define CONFIG_INIT_ENTRY 1
#  endif

#  if defined(CONFIG_INIT_ENTRY)
  /* Initialize by starting a task at an entry point */

#    ifndef CONFIG_INIT_ENTRYPOINT
  /* Entry point name must have been provided */

#      error CONFIG_INIT_ENTRYPOINT must be defined
#    endif

#  elif defined(CONFIG_INIT_FILE)
  /* Initialize by running an initialization program in the file system.
   * Presumably the user has configured a board initialization function
   * that will mount the file system containing the initialization
   * program.
   */

#    ifndef CONFIG_INIT_FILEPATH
  /* Path to the initialization program must have been provided */

#      error CONFIG_INT_FILEPATH must be defined
#    endif

#    if !defined(CONFIG_INIT_SYMTAB) || !defined(CONFIG_INIT_NEXPORTS)
  /* No symbol information... assume no symbol table is available */

#      undef CONFIG_INIT_SYMTAB
#      undef CONFIG_INIT_NEXPORTS
#      define CONFIG_INIT_SYMTAB NULL
#      define CONFIG_INIT_NEXPORTS 0
#    else
extern const struct symtab_s CONFIG_INIT_SYMTAB[];
extern const int             CONFIG_INIT_NEXPORTS;
#    endif
#  endif
#endif

/* In the protected build (only) we also need to start the user work queue */

#if !defined(CONFIG_BUILD_PROTECTED)
#  undef CONFIG_LIBC_USRWORK
#endif

#if !defined(CONFIG_INIT_PRIORITY)
#  define CONFIG_INIT_PRIORITY SCHED_PRIORITY_DEFAULT
#endif

#ifdef CONFIG_ETC_ROMFS
#  define NSECTORS(b)        (((b)+CONFIG_ETC_ROMFSSECTSIZE-1)/CONFIG_ETC_ROMFSSECTSIZE)
#  define MKMOUNT_DEVNAME(m) "/dev/ram" STRINGIFY(m)
#  define MOUNT_DEVNAME      MKMOUNT_DEVNAME(CONFIG_ETC_ROMFSDEVNO)

extern const unsigned char romfs_img[];
extern const unsigned int romfs_img_len;
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nx_pgworker
 *
 * Description:
 *   Start the page fill worker kernel thread that will resolve page faults.
 *   This should always be the first thread started because it may have to
 *   resolve page faults in other threads
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

#ifdef CONFIG_LEGACY_PAGING
static inline void nx_pgworker(void)
{
  /* Start the page fill worker kernel thread that will resolve page faults.
   * This should always be the first thread started because it may have to
   * resolve page faults in other threads
   */

  sinfo("Starting paging thread\n");

  g_pgworker = kthread_create("pgfill", CONFIG_PAGING_DEFPRIO,
                              CONFIG_PAGING_STACKSIZE,
                              pg_worker, NULL);
  DEBUGASSERT(g_pgworker > 0);
}

#else /* CONFIG_LEGACY_PAGING */
#  define nx_pgworker()

#endif /* CONFIG_LEGACY_PAGING */

/****************************************************************************
 * Name: nx_workqueues
 *
 * Description:
 *   Start the worker threads that service the work queues.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

#ifdef CONFIG_SCHED_WORKQUEUE
static inline void nx_workqueues(void)
{
#ifdef CONFIG_LIBC_USRWORK
  pid_t pid;
#endif

#ifdef CONFIG_SCHED_HPWORK
  /* Start the high-priority worker thread to support device driver lower
   * halves.
   */

  work_start_highpri();

#endif /* CONFIG_SCHED_HPWORK */

#ifdef CONFIG_SCHED_LPWORK
  /* Start the low-priority worker thread for other, non-critical
   * continuation tasks
   */

  work_start_lowpri();

#endif /* CONFIG_SCHED_LPWORK */

#ifdef CONFIG_LIBC_USRWORK
  /* Start the user-space work queue */

  DEBUGASSERT(USERSPACE->work_usrstart != NULL);
  pid = USERSPACE->work_usrstart();
  DEBUGASSERT(pid > 0);
  UNUSED(pid);
#endif
}

#else /* CONFIG_SCHED_WORKQUEUE */
#  define nx_workqueues()

#endif /* CONFIG_SCHED_WORKQUEUE */

/****************************************************************************
 * Name: nx_romfsetc
 *
 * Description: mount baked-in ROMFS image to /etc.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

#ifdef CONFIG_ETC_ROMFS
static inline void nx_romfsetc(void)
{
  int ret;

#ifndef CONFIG_ETC_CROMFS
  /* Create a ROM disk for the /etc filesystem */

  ret = romdisk_register(CONFIG_ETC_ROMFSDEVNO, romfs_img,
                         NSECTORS(romfs_img_len),
                         CONFIG_ETC_ROMFSSECTSIZE);
  if (ret < 0)
    {
      ferr("ERROR: romdisk_register failed: %d\n", -ret);
      return;
    }
#endif

  /* Mount the file system */

  finfo("Mounting ROMFS filesystem at target=%s with source=%s\n",
        CONFIG_ETC_ROMFSMOUNTPT, MOUNT_DEVNAME);

#if defined(CONFIG_ETC_CROMFS)
  ret = nx_mount(MOUNT_DEVNAME, CONFIG_ETC_ROMFSMOUNTPT,
                 "cromfs", MS_RDONLY, NULL);
#else
  ret = nx_mount(MOUNT_DEVNAME, CONFIG_ETC_ROMFSMOUNTPT,
                 "romfs", MS_RDONLY, NULL);
#endif
  if (ret < 0)
    {
      ferr("ERROR: nx_mount(%s,%s,romfs) failed: %d\n",
           MOUNT_DEVNAME, CONFIG_ETC_ROMFSMOUNTPT, ret);
    }
}

#endif /* CONFIG_ETC_ROMFS */

/****************************************************************************
 * Name: nx_start_application
 *
 * Description:
 *   Execute the board initialization function (if so configured) and start
 *   the application initialization thread.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static inline void nx_start_application(void)
{
#ifndef CONFIG_INIT_NONE
  FAR char * const argv[] =
  {
#  ifdef CONFIG_INIT_ARGS
    CONFIG_INIT_ARGS,
#  endif
    NULL,
  };

  posix_spawnattr_t attr;
#endif
  int ret;

#ifdef CONFIG_ETC_ROMFS
  nx_romfsetc();
#endif

#ifdef CONFIG_BOARD_LATE_INITIALIZE
  /* Perform any last-minute, board-specific initialization, if so
   * configured.
   */

  board_late_initialize();
#endif

#ifdef CONFIG_COREDUMP
  coredump_initialize();
#endif

  posix_spawnattr_init(&attr);
  attr.priority  = CONFIG_INIT_PRIORITY;
  attr.stacksize = CONFIG_INIT_STACKSIZE;

#if defined(CONFIG_INIT_ENTRY)

  /* Start the application initialization task.  In a flat build, this is
   * entrypoint is given by the definitions, CONFIG_INIT_ENTRYPOINT.  In
   * the protected build, however, we must get the address of the
   * entrypoint from the header at the beginning of the user-space blob.
   */

  sinfo("Starting init thread\n");

#  ifdef CONFIG_BUILD_PROTECTED
  DEBUGASSERT(USERSPACE->us_entrypoint != NULL);
  ret = task_spawn(CONFIG_INIT_ENTRYNAME,
                   USERSPACE->us_entrypoint,
                   NULL, &attr, argv, NULL);
#  else
  ret = task_spawn(CONFIG_INIT_ENTRYNAME,
                   CONFIG_INIT_ENTRYPOINT,
                   NULL, &attr, argv, NULL);
#  endif
#elif defined(CONFIG_INIT_FILE)

#  ifdef CONFIG_INIT_MOUNT
  /* Mount the file system containing the init program. */

  ret = nx_mount(CONFIG_INIT_MOUNT_SOURCE, CONFIG_INIT_MOUNT_TARGET,
                 CONFIG_INIT_MOUNT_FSTYPE, CONFIG_INIT_MOUNT_FLAGS,
                 CONFIG_INIT_MOUNT_DATA);
  DEBUGASSERT(ret >= 0);
#  endif

  /* Start the application initialization program from a program in a
   * mounted file system.  Presumably the file system was mounted as part
   * of the board_late_initialize() operation.
   */

  sinfo("Starting init task: %s\n", CONFIG_INIT_FILEPATH);

  posix_spawnattr_init(&attr);

  attr.priority  = CONFIG_INIT_PRIORITY;
  attr.stacksize = CONFIG_INIT_STACKSIZE;

  ret = exec_spawn(CONFIG_INIT_FILEPATH, argv, NULL,
                   CONFIG_INIT_SYMTAB, CONFIG_INIT_NEXPORTS, NULL, &attr);
#endif
  posix_spawnattr_destroy(&attr);
  DEBUGASSERT(ret > 0);
}

/****************************************************************************
 * Name: nx_start_task
 *
 * Description:
 *   This is the framework for a short duration worker thread.  It off-loads
 *   the board initialization and application start-up from the limited
 *   start-up, initialization thread to a more robust kernel thread.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

#ifdef CONFIG_BOARD_LATE_INITIALIZE
static int nx_start_task(int argc, FAR char **argv)
{
  /* Do the board/application initialization and exit */

  nx_start_application();
  return OK;
}
#endif

/****************************************************************************
 * Name: nx_create_initthread
 *
 * Description:
 *   Execute the board initialization function (if so configured) and start
 *   the application initialization thread.  This will be done either on the
 *   thread of execution of the caller or on a separate thread of execution
 *   if so configured.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static inline void nx_create_initthread(void)
{
#ifdef CONFIG_BOARD_LATE_INITIALIZE
  int pid;

  /* Do the board/application initialization on a separate thread of
   * execution.
   */

  pid = nxthread_create("AppBringUp", TCB_FLAG_TTYPE_KERNEL,
                        CONFIG_BOARD_INITTHREAD_PRIORITY,
                        NULL, CONFIG_BOARD_INITTHREAD_STACKSIZE,
                        nx_start_task, NULL, environ);
  DEBUGASSERT(pid > 0);
  UNUSED(pid);
#else
  /* Do the board/application initialization on this thread of execution. */

  nx_start_application();
#endif
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nx_bringup
 *
 * Description:
 *   Start all initial system tasks.  This does the "system bring-up" after
 *   the conclusion of basic OS initialization.  These initial system tasks
 *   may include:
 *
 *   - pg_worker:   The page-fault worker thread (if CONFIG_LEGACY_PAGING is
 *                  defined).
 *   - work_thread: The work thread.  This general thread can be used to
 *                  perform most any kind of queued work.  Its primary
 *                  function is to serve as the "bottom half" of device
 *                  drivers.
 *
 *   And the main application entry point:
 *   symbols, either:
 *
 *   - CONFIG_INIT_ENTRYPOINT: This is the default user application entry
 *                 point, or
 *   - CONFIG_INIT_FILEPATH: The full path to the location in a mounted
 *                 file system where we can expect to find the
 *                 initialization program.  Presumably, this file system
 *                 was mounted by board-specific logic when
 *                 board_late_initialize() was called.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

int nx_bringup(void)
{
  sched_trace_begin();

#ifndef CONFIG_DISABLE_ENVIRON
  /* Setup up the initial environment for the idle task.  At present, this
   * may consist of only the initial PATH variable and/or and init library
   * path variable.  These path variables are not used by the IDLE task.
   * However, the environment containing the PATH variable will be inherited
   * by all of the threads created by the IDLE task.
   */

#ifdef CONFIG_LIBC_HOMEDIR
  setenv("PWD", CONFIG_LIBC_HOMEDIR, 1);
#endif

#ifdef CONFIG_PATH_INITIAL
  setenv("PATH", CONFIG_PATH_INITIAL, 1);
#endif

#ifdef CONFIG_LDPATH_INITIAL
  setenv("LD_LIBRARY_PATH", CONFIG_LDPATH_INITIAL, 1);
#endif
#endif

  /* Start the page fill worker kernel thread that will resolve page faults.
   * This should always be the first thread started because it may have to
   * resolve page faults in other threads
   */

  nx_pgworker();

  /* Start the worker thread that will serve as the device driver "bottom-
   * half" and will perform misc garbage clean-up.
   */

  nx_workqueues();

  /* Once the operating system has been initialized, the system must be
   * started by spawning the user initialization thread of execution.  This
   * will be the first user-mode thread.
   */

  nx_create_initthread();

#if !defined(CONFIG_DISABLE_ENVIRON) && (defined(CONFIG_PATH_INITIAL) || \
     defined(CONFIG_LDPATH_INITIAL))
  /* We would save a few bytes by discarding the IDLE thread's environment.
   * But when kthreads share the same group, this is no longer proper, so
   * we can't do clearenv() now.
   */

#endif

  sched_trace_end();
  return OK;
}
