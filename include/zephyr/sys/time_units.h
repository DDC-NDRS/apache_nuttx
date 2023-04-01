/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_TIME_UNITS_H_
#define ZEPHYR_INCLUDE_TIME_UNITS_H_

#include <zephyr/toolchain.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief System-wide macro to denote "forever" in milliseconds
 *
 *  Usage of this macro is limited to APIs that want to expose a timeout value
 *  that can optionally be unlimited, or "forever".
 *  This macro can not be fed into kernel functions or macros directly. Use
 *  @ref SYS_TIMEOUT_MS instead.
 */
#define SYS_FOREVER_MS (-1)

/** @brief System-wide macro to denote "forever" in microseconds
 *
 * See @ref SYS_FOREVER_MS.
 */
#define SYS_FOREVER_US (-1)

/** @brief System-wide macro to convert milliseconds to kernel timeouts
 */
#define SYS_TIMEOUT_MS(ms) ((ms) == SYS_FOREVER_MS ? K_FOREVER : K_MSEC(ms))

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ZEPHYR_INCLUDE_TIME_UNITS_H_ */
