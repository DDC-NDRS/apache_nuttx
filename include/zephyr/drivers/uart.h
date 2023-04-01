/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for UART drivers
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_UART_H_
#define ZEPHYR_INCLUDE_DRIVERS_UART_H_

/**
 * @brief UART Interface
 * @defgroup uart_interface UART Interface
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>
#include <stddef.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/drivers/uart_common.h>

#if defined(CONFIG_ARCH_CHIP_S32K14X)
#include <zephyr/drivers/s32k1xx/s32k1xx_uart.h>
#endif

#if defined(CONFIG_ARCH_CHIP_STM32H7)
#include <zephyr/drivers/stm32h7/stm32h7_uart.h>
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_UART_H_ */
