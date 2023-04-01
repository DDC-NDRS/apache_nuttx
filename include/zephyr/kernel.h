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

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <zephyr/toolchain.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int32_t k_usleep(int32_t us) {
    int ret;

    ret = usleep((useconds_t)us);
    return (int32_t)(ret);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_KERNEL_H_ */
