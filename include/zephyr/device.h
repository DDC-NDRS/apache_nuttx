/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEVICE_H_
#define ZEPHYR_INCLUDE_DEVICE_H_

#include <stdint.h>
#include <zephyr/sys/util.h>

#if defined(_MSC_VER)                       /* #CUSTOM@NDRS */
#define DT_CONST
#else
#define DT_CONST    const
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Device Model
 * @defgroup device_model Device Model
 * @{
 */

/**
 * @brief Type used to represent a "handle" for a device.
 *
 * Every @ref device has an associated handle. You can get a pointer to a
 * @ref device from its handle and vice versa, but the handle uses less space
 * than a pointer. The device.h API mainly uses handles to store lists of
 * multiple devices in a compact way.
 *
 * The extreme values and zero have special significance. Negative values
 * identify functionality that does not correspond to a Zephyr device, such as
 * the system clock or a SYS_INIT() function.
 *
 * @see device_handle_get()
 * @see device_from_handle()
 */
typedef int16_t device_handle_t;

/**
 * @brief Flag value used in lists of device handles to separate distinct
 * groups.
 *
 * This is the minimum value for the device_handle_t type.
 */
#define DEVICE_HANDLE_SEP INT16_MIN

/**
 * @brief Flag value used in lists of device handles to indicate the end of the
 * list.
 *
 * This is the maximum value for the device_handle_t type.
 */
#define DEVICE_HANDLE_ENDS INT16_MAX

/** @brief Flag value used to identify an unknown device. */
#define DEVICE_HANDLE_NULL 0

/**
 * @brief Runtime device dynamic structure (in RAM) per driver instance
 *
 * Fields in this are expected to be default-initialized to zero. The
 * kernel driver infrastructure and driver access functions are
 * responsible for ensuring that any non-zero initialization is done
 * before they are accessed.
 */
struct device_state {
	/**
	 * Device initialization return code (positive errno value).
	 *
	 * Device initialization functions return a negative errno code if they
	 * fail. In Zephyr, errno values do not exceed 255, so we can store the
	 * positive result value in a uint8_t type.
	 */
	uint8_t init_res;

	/** Indicates the device initialization function has been
	 * invoked.
	 */
	bool initialized : 1;
};

struct pm_device;

/**
 * @brief Runtime device structure (in ROM) per driver instance
 */
struct device {
	/** Name of the device instance */
	const char* name;
	/** Address of device instance config information */
	const void* config;
	/** Address of the API structure exposed by the device instance */
	const void* api;
	/** Address of the common device state */
	struct device_state* state;
	/** Address of the device instance private data */
    void* data;
	/**
	 * Optional pointer to handles associated with the device.
	 *
	 * This encodes a sequence of sets of device handles that have some
	 * relationship to this node. The individual sets are extracted with
	 * dedicated API, such as device_required_handles_get().
	 */
	device_handle_t* handles;

#if defined(CONFIG_PM_DEVICE) || defined(__DOXYGEN__)
	/**
	 * Reference to the device PM resources (only available if
	 * @kconfig{CONFIG_PM_DEVICE} is enabled).
	 */
	struct pm_device *pm;
#endif
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DEVICE_H_ */
