/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 * Copyright (c) 2023 NDR Solution (Thailand) Co., Ltd.
 *
 * Heavily based on drivers/flash.h which is:
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for FRAM drivers
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_FRAM_H_
#define ZEPHYR_INCLUDE_DRIVERS_FRAM_H_

/**
 * @brief FRAM Interface
 * @defgroup fram_interface FRAM Interface
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <sys/types.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  @brief Read data from FRAM
 *
 *  @param dev FRAM device
 *  @param offset Address offset to read from.
 *  @param data Buffer to store read data.
 *  @param len Number of bytes to read.
 *
 *  @return 0 on success, negative errno code on failure.
 */
static inline int fram_read(const struct device* dev, off_t offset, void* data, size_t len) {
    return (0);
}

/**
 *  @brief Write data to FRAM
 *
 *  @param dev FRAM device
 *  @param offset Address offset to write data to.
 *  @param data Buffer with data to write.
 *  @param len Number of bytes to write.
 *
 *  @return 0 on success, negative errno code on failure.
 */
static inline int fram_write(const struct device* dev, off_t offset, const void* data, size_t len) {
    return (0);
}

/**
 *  @brief Get the size of the FRAM in bytes
 *
 *  @param dev FRAM device.
 *
 *  @return FRAM size in bytes.
 */
static inline size_t fram_get_size(const struct device* dev) {
    return (0UL);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_FRAM_H_ */
