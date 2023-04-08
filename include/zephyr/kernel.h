/*
 * Copyright (c) 2016, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Public kernel APIs.
 */

#ifndef ZEPHYR_INCLUDE_KERNEL_H_
#define ZEPHYR_INCLUDE_KERNEL_H_

#include <nuttx/config.h>
#include <nuttx/clock.h>
#include <nuttx/wqueue.h>
#include <pthread.h>
#include <errno.h>
#include <poll.h>
#include <fcntl.h>

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include <zephyr/toolchain.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/sys_clock.h>
#include <zephyr/kernel_structs.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ARCH_KERNEL_STACK_RESERVED
#define K_KERNEL_STACK_RESERVED	((size_t)ARCH_KERNEL_STACK_RESERVED)
#else
#define K_KERNEL_STACK_RESERVED	((size_t)0)
#endif
#define K_KERNEL_STACK_SIZEOF(sym) (sizeof(sym) - K_KERNEL_STACK_RESERVED)
#define K_THREAD_STACK_SIZEOF		K_KERNEL_STACK_SIZEOF

/**
 * @addtogroup clock_apis
 * @{
 */

/**
 * @brief Generate null timeout delay.
 *
 * This macro generates a timeout delay that instructs a kernel API
 * not to wait if the requested operation cannot be performed immediately.
 *
 * @return Timeout delay value.
 */
#define K_NO_WAIT Z_TIMEOUT_NO_WAIT

/**
 * @brief Generate timeout delay from nanoseconds.
 *
 * This macro generates a timeout delay that instructs a kernel API to
 * wait up to @a t nanoseconds to perform the requested operation.
 * Note that timer precision is limited to the tick rate, not the
 * requested value.
 *
 * @param t Duration in nanoseconds.
 *
 * @return Timeout delay value.
 */
#define K_NSEC(t)     Z_TIMEOUT_NS(t)

/**
 * @brief Generate timeout delay from microseconds.
 *
 * This macro generates a timeout delay that instructs a kernel API
 * to wait up to @a t microseconds to perform the requested operation.
 * Note that timer precision is limited to the tick rate, not the
 * requested value.
 *
 * @param t Duration in microseconds.
 *
 * @return Timeout delay value.
 */
#define K_USEC(t)     Z_TIMEOUT_US(t)

/**
 * @brief Generate timeout delay from cycles.
 *
 * This macro generates a timeout delay that instructs a kernel API
 * to wait up to @a t cycles to perform the requested operation.
 *
 * @param t Duration in cycles.
 *
 * @return Timeout delay value.
 */
#define K_CYC(t)     Z_TIMEOUT_CYC(t)

/**
 * @brief Generate timeout delay from system ticks.
 *
 * This macro generates a timeout delay that instructs a kernel API
 * to wait up to @a t ticks to perform the requested operation.
 *
 * @param t Duration in system ticks.
 *
 * @return Timeout delay value.
 */
#define K_TICKS(t)     Z_TIMEOUT_TICKS(t)

/**
 * @brief Generate timeout delay from milliseconds.
 *
 * This macro generates a timeout delay that instructs a kernel API
 * to wait up to @a ms milliseconds to perform the requested operation.
 *
 * @param ms Duration in milliseconds.
 *
 * @return Timeout delay value.
 */
#define K_MSEC(ms)     Z_TIMEOUT_MS(ms)

/**
 * @brief Generate timeout delay from seconds.
 *
 * This macro generates a timeout delay that instructs a kernel API
 * to wait up to @a s seconds to perform the requested operation.
 *
 * @param s Duration in seconds.
 *
 * @return Timeout delay value.
 */
#define K_SECONDS(s)   K_MSEC((s) * MSEC_PER_SEC)

/**
 * @brief Generate timeout delay from minutes.

 * This macro generates a timeout delay that instructs a kernel API
 * to wait up to @a m minutes to perform the requested operation.
 *
 * @param m Duration in minutes.
 *
 * @return Timeout delay value.
 */
#define K_MINUTES(m)   K_SECONDS((m) * 60)

/**
 * @brief Generate timeout delay from hours.
 *
 * This macro generates a timeout delay that instructs a kernel API
 * to wait up to @a h hours to perform the requested operation.
 *
 * @param h Duration in hours.
 *
 * @return Timeout delay value.
 */
#define K_HOURS(h)     K_MINUTES((h) * 60)

/**
 * @brief Generate infinite timeout delay.
 *
 * This macro generates a timeout delay that instructs a kernel API
 * to wait as long as necessary to perform the requested operation.
 *
 * @return Timeout delay value.
 */
#define K_FOREVER Z_FOREVER

static inline int32_t k_usleep(int32_t us) {
    int ret;

    ret = usleep((useconds_t)us);
    return (int32_t)(ret);
}

static inline int64_t k_uptime_ticks(void) {
	return ((int64_t)clock_systime_ticks());
}

/**
 * @cond INTERNAL_HIDDEN
 */
struct k_work_delayable;
struct k_work_sync;

/**
 * @defgroup workqueue_apis Work Queue APIs
 * @ingroup kernel_apis
 * @{
 */

/** @brief The signature for a work item handler function.
 *
 * The function will be invoked by the thread animating a work queue.
 *
 * @param work the work item that provided the handler.
 */
typedef worker_t k_work_handler_t;

enum {
/**
 * @cond INTERNAL_HIDDEN
 */

/* The atomic API is used for all work and queue flags fields to
 * enforce sequential consistency in SMP environments.
 */

/* Bits that represent the work item states.  At least nine of the
 * combinations are distinct valid stable states.
 */
K_WORK_RUNNING_BIT = 0,
K_WORK_CANCELING_BIT = 1,
K_WORK_QUEUED_BIT = 2,
K_WORK_DELAYED_BIT = 3,

K_WORK_MASK = BIT(K_WORK_DELAYED_BIT) | BIT(K_WORK_QUEUED_BIT) | BIT(K_WORK_RUNNING_BIT) | BIT(K_WORK_CANCELING_BIT),

/* Static work flags */
K_WORK_DELAYABLE_BIT = 8,
K_WORK_DELAYABLE = BIT(K_WORK_DELAYABLE_BIT),

/* Dynamic work queue flags */
K_WORK_QUEUE_STARTED_BIT = 0,
K_WORK_QUEUE_STARTED = BIT(K_WORK_QUEUE_STARTED_BIT),
K_WORK_QUEUE_BUSY_BIT = 1,
K_WORK_QUEUE_BUSY = BIT(K_WORK_QUEUE_BUSY_BIT),
K_WORK_QUEUE_DRAIN_BIT = 2,
K_WORK_QUEUE_DRAIN = BIT(K_WORK_QUEUE_DRAIN_BIT),
K_WORK_QUEUE_PLUGGED_BIT = 3,
K_WORK_QUEUE_PLUGGED = BIT(K_WORK_QUEUE_PLUGGED_BIT),

/* Static work queue flags */
K_WORK_QUEUE_NO_YIELD_BIT = 8,
K_WORK_QUEUE_NO_YIELD = BIT(K_WORK_QUEUE_NO_YIELD_BIT),

/**
 * INTERNAL_HIDDEN @endcond
 */
/* Transient work flags */

/** @brief Flag indicating a work item that is running under a work
 * queue thread.
 *
 * Accessed via k_work_busy_get().  May co-occur with other flags.
 */
K_WORK_RUNNING = BIT(K_WORK_RUNNING_BIT),

/** @brief Flag indicating a work item that is being canceled.
 *
 * Accessed via k_work_busy_get().  May co-occur with other flags.
 */
K_WORK_CANCELING = BIT(K_WORK_CANCELING_BIT),

/** @brief Flag indicating a work item that has been submitted to a
 * queue but has not started running.
 *
 * Accessed via k_work_busy_get().  May co-occur with other flags.
 */
K_WORK_QUEUED = BIT(K_WORK_QUEUED_BIT),

/** @brief Flag indicating a delayed work item that is scheduled for
 * submission to a queue.
 *
 * Accessed via k_work_busy_get().  May co-occur with other flags.
 */
K_WORK_DELAYED = BIT(K_WORK_DELAYED_BIT),
};

/** @brief A structure used to submit work. */
struct /**/k_work {
	/* All fields are protected by the work module spinlock.  No fields
	 * are to be accessed except through kernel API.
	 */

	/* Node to link into k_work_q pending list. */
	sys_snode_t node;

	/* The function to be invoked by the work queue thread. */
	k_work_handler_t handler;

	/* The queue on which the work item was last submitted. */
	struct k_work_q *queue;

	/* State of the work item.
	 *
	 * The item can be DELAYED, QUEUED, and RUNNING simultaneously.
	 *
	 * It can be RUNNING and CANCELING simultaneously.
	 */
	uint32_t flags;
};

#define Z_WORK_INITIALIZER(work_handler) { \
	.handler = work_handler, \
}

/** @brief A structure used to submit work after a delay. */
struct /**/k_work_delayable {
	/* The work item. */
	#if defined(__ZEPHYR__)
	struct k_work work;
	#else
	struct work_s work;
	#endif

	/* Timeout used to submit work after a delay. */
	struct _timeout timeout;

	/* The queue to which the work should be submitted. */
	struct k_work_q *queue;
};

#define Z_WORK_DELAYABLE_INITIALIZER(work_handler) { \
	.work = { \
		.handler = work_handler, \
		.flags = K_WORK_DELAYABLE, \
	}, \
}

/** @brief Initialize a (non-delayable) work structure.
 *
 * This must be invoked before submitting a work structure for the first time.
 * It need not be invoked again on the same work structure.  It can be
 * re-invoked to change the associated handler, but this must be done when the
 * work item is idle.
 *
 * @funcprops \isr_ok
 *
 * @param work the work structure to be initialized.
 *
 * @param handler the handler to be invoked by the work item.
 */
void k_work_init(struct k_work* work,
                 k_work_handler_t handler);

/** @brief Initialize a delayable work structure.
 *
 * This must be invoked before scheduling a delayable work structure for the
 * first time.  It need not be invoked again on the same work structure.  It
 * can be re-invoked to change the associated handler, but this must be done
 * when the work item is idle.
 *
 * @funcprops \isr_ok
 *
 * @param dwork the delayable work structure to be initialized.
 *
 * @param handler the handler to be invoked by the work item.
 */
static inline void k_work_init_delayable(struct k_work_delayable* dwork,
                           				 k_work_handler_t handler) {
	(void) memset(&dwork->work, 0, sizeof(dwork->work));
    dwork->work.worker = handler;
}

/** @brief Submit an idle work item to the system work queue after a
 * delay.
 *
 * This is a thin wrapper around k_work_schedule_for_queue(), with all the API
 * characteristics of that function.
 *
 * @param dwork pointer to the delayable work item.
 *
 * @param delay the time to wait before submitting the work item.  If @c
 * K_NO_WAIT this is equivalent to k_work_submit_to_queue().
 *
 * @return as with k_work_schedule_for_queue().
 */
static inline int k_work_schedule(struct k_work_delayable* dwork,
                                  k_timeout_t delay) {
    /* Do the trick since work_queue will check dwork->work.worker to be NULL before */
    worker_t worker = dwork->work.worker;
	dwork->work.worker = NULL;

    int ret = work_queue(LPWORK, &dwork->work, worker, dwork, delay.ticks);

    return (ret);
}

/**
 * @brief Cause the current thread to busy wait.
 *
 * This routine causes the current thread to execute a "do nothing" loop for
 * @a usec_to_wait microseconds.
 *
 * @note The clock used for the microsecond-resolution delay here may
 * be skewed relative to the clock used for system timeouts like
 * k_sleep().  For example k_busy_wait(1000) may take slightly more or
 * less time than k_sleep(K_MSEC(1)), with the offset dependent on
 * clock tolerances.
 */
static inline void k_busy_wait(uint32_t usec_to_wait) {
    (void) usleep(usec_to_wait);
}

// NuttX equivalent for k_poll_event structure
typedef struct pollfd k_poll_event;

static inline void k_poll_event_init(k_poll_event* event, uint32_t type, int mode, void* obj) {
	event->arg = obj;
}

static inline int k_poll(k_poll_event* events, int num_events, k_timeout_t timeout) {
	int ret = poll(events, num_events, (timeout.ticks / TICK_PER_MSEC));

	return (ret);
}

static inline void k_poll_signal_init(struct k_poll_signal* sig) {

}

static inline int k_poll_signal_raise(struct k_poll_signal* sig, int result) {
	return (0);
}

/* LINES 5,000+ */
/* public - polling modes */
/* polling API - PRIVATE */

#ifdef CONFIG_POLL
#define _INIT_OBJ_POLL_EVENT(obj) do { (obj)->poll_event = NULL; } while (false)
#else
#define _INIT_OBJ_POLL_EVENT(obj) do { } while (false)
#endif

/* private - types bit positions */
enum _poll_types_bits {
    /* can be used to ignore an event */
    _POLL_TYPE_IGNORE,

    /* to be signaled by k_poll_signal_raise() */
    _POLL_TYPE_SIGNAL,

    /* semaphore availability */
    _POLL_TYPE_SEM_AVAILABLE,

    /* queue/FIFO/LIFO data availability */
    _POLL_TYPE_DATA_AVAILABLE,

    /* msgq data availability */
    _POLL_TYPE_MSGQ_DATA_AVAILABLE,

    /* pipe data availability */
    _POLL_TYPE_PIPE_DATA_AVAILABLE,

    _POLL_NUM_TYPES
};

#define Z_POLL_TYPE_BIT(type) (1U << ((type) - 1U))

/* private - states bit positions */
enum _poll_states_bits {
    /* default state when creating event */
    _POLL_STATE_NOT_READY,

    /* signaled by k_poll_signal_raise() */
    _POLL_STATE_SIGNALED,

    /* semaphore is available */
    _POLL_STATE_SEM_AVAILABLE,

    /* data is available to read on queue/FIFO/LIFO */
    _POLL_STATE_DATA_AVAILABLE,

    /* queue/FIFO/LIFO wait was cancelled */
    _POLL_STATE_CANCELLED,

    /* data is available to read on a message queue */
    _POLL_STATE_MSGQ_DATA_AVAILABLE,

    /* data is available to read from a pipe */
    _POLL_STATE_PIPE_DATA_AVAILABLE,

    _POLL_NUM_STATES
};

#define Z_POLL_STATE_BIT(state) (1U << ((state) - 1U))

/* public - values for k_poll_event.type bitfield */
#define K_POLL_TYPE_IGNORE 0
#define K_POLL_TYPE_SIGNAL Z_POLL_TYPE_BIT(_POLL_TYPE_SIGNAL)
#define K_POLL_TYPE_SEM_AVAILABLE Z_POLL_TYPE_BIT(_POLL_TYPE_SEM_AVAILABLE)
#define K_POLL_TYPE_DATA_AVAILABLE Z_POLL_TYPE_BIT(_POLL_TYPE_DATA_AVAILABLE)
#define K_POLL_TYPE_FIFO_DATA_AVAILABLE K_POLL_TYPE_DATA_AVAILABLE
#define K_POLL_TYPE_MSGQ_DATA_AVAILABLE Z_POLL_TYPE_BIT(_POLL_TYPE_MSGQ_DATA_AVAILABLE)
#define K_POLL_TYPE_PIPE_DATA_AVAILABLE Z_POLL_TYPE_BIT(_POLL_TYPE_PIPE_DATA_AVAILABLE)

/* public - polling modes */
enum k_poll_modes {
    /* polling thread does not take ownership of objects when available */
    K_POLL_MODE_NOTIFY_ONLY = 0,

    K_POLL_NUM_MODES
};

/* public - values for k_poll_event.state bitfield */
#define K_POLL_STATE_NOT_READY 0
#define K_POLL_STATE_SIGNALED Z_POLL_STATE_BIT(_POLL_STATE_SIGNALED)
#define K_POLL_STATE_SEM_AVAILABLE Z_POLL_STATE_BIT(_POLL_STATE_SEM_AVAILABLE)
#define K_POLL_STATE_DATA_AVAILABLE Z_POLL_STATE_BIT(_POLL_STATE_DATA_AVAILABLE)
#define K_POLL_STATE_FIFO_DATA_AVAILABLE K_POLL_STATE_DATA_AVAILABLE
#define K_POLL_STATE_MSGQ_DATA_AVAILABLE Z_POLL_STATE_BIT(_POLL_STATE_MSGQ_DATA_AVAILABLE)
#define K_POLL_STATE_PIPE_DATA_AVAILABLE Z_POLL_STATE_BIT(_POLL_STATE_PIPE_DATA_AVAILABLE)
#define K_POLL_STATE_CANCELLED Z_POLL_STATE_BIT(_POLL_STATE_CANCELLED)

/* public - poll signal object */
struct k_poll_signal {
    /** PRIVATE - DO NOT TOUCH */
    sys_dlist_t poll_events;

    /**
     * 1 if the event has been signaled, 0 otherwise. Stays set to 1 until
     * user resets it to 0.
     */
    unsigned int signaled;

    /** custom result value passed to k_poll_signal_raise() if needed */
    int result;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_KERNEL_H_ */
