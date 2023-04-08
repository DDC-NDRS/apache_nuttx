/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * Public APIs for pin control drivers
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_STM32H7_PINCTRL_H_
#define ZEPHYR_INCLUDE_DRIVERS_STM32H7_PINCTRL_H_

/**
 * @brief Pin Controller Interface
 * @defgroup pinctrl_interface Pin Controller Interface
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>
#include <nuttx/nuttx.h>
#include <nuttx/config.h>
#include <nuttx/board.h>
#include <arch/arm/src/stm32h7/stm32_gpio.h>

#include <zephyr/drivers/pinctrl.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Find the state configuration for the given state id.
 *
 * @param config Pin controller configuration.
 * @param id Pin controller state id (see @ref PINCTRL_STATES).
 * @param state Found state.
 *
 * @retval 0 If state has been found.
 * @retval -ENOENT If the state has not been found.
 */
int pinctrl_lookup_state(const struct pinctrl_dev_config* config, uint8_t id,
                         const struct pinctrl_state** state);

/**
 * @brief Configure a set of pins.
 *
 * This function will configure the necessary hardware blocks to make the
 * configuration immediately effective.
 *
 * @warning This function must never be used to configure pins used by an
 * instantiated device driver.
 *
 * @param pins List of pins to be configured.
 * @param pin_cnt Number of pins.
 * @param reg Device register (optional, use #PINCTRL_REG_NONE if not used).
 *
 * @retval 0 If succeeded
 * @retval -errno Negative errno for other failures.
 */
int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
                           uintptr_t reg);

/**
 * @brief Apply a state directly from the provided state configuration.
 *
 * @param config Pin control configuration.
 * @param state State.
 *
 * @retval 0 If succeeded
 * @retval -errno Negative errno for other failures.
 */
static inline int pinctrl_apply_state_direct(const struct pinctrl_dev_config* config,
											 const struct pinctrl_state* state) {
	// Just minimum implementation for now
	int ret = stm32_configgpio(state->pins[0]);

	return (ret);
}

/**
 * @brief Apply a state from the given device configuration.
 *
 * @param config Pin control configuration.
 * @param id Id of the state to be applied (see @ref PINCTRL_STATES).
 *
 * @retval 0 If succeeded.
 * @retval -ENOENT If given state id does not exist.
 * @retval -errno Negative errno for other failures.
 */
static inline int pinctrl_apply_state(const struct pinctrl_dev_config* config, uint8_t id) {
	return pinctrl_apply_state_direct(config, config->states);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_STM32H7_PINCTRL_H_ */
