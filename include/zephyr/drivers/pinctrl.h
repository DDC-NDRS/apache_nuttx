/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * Public APIs for pin control drivers
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PINCTRL_H_
#define ZEPHYR_INCLUDE_DRIVERS_PINCTRL_H_

/**
 * @brief Pin Controller Interface
 * @defgroup pinctrl_interface Pin Controller Interface
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name Pin control states
 * @anchor PINCTRL_STATES
 * @{
 */

/** Default state (state used when the device is in operational state). */
#define PINCTRL_STATE_DEFAULT 0U
/** Sleep state (state used when the device is in low power mode). */
#define PINCTRL_STATE_SLEEP 1U

/** This and higher values refer to custom private states. */
#define PINCTRL_STATE_PRIV_START 2U

/** @} */
typedef uint32_t pinctrl_soc_pin_t;

/** Pin control state configuration. */
struct pinctrl_state {
	/** Pin configurations. */
	const pinctrl_soc_pin_t* pins;
	/** Number of pin configurations. */
	uint8_t pin_cnt;
	/** State identifier (see @ref PINCTRL_STATES). */
	uint8_t id;
};

/** Pin controller configuration for a given device. */
struct pinctrl_dev_config {
#if defined(CONFIG_PINCTRL_STORE_REG) || defined(__DOXYGEN__)
	/**
	 * Device address (only available if @kconfig{CONFIG_PINCTRL_STORE_REG}
	 * is enabled).
	 */
	uintptr_t reg;
#endif /* defined(CONFIG_PINCTRL_STORE_REG) || defined(__DOXYGEN__) */
	/** List of state configurations. */
	const struct pinctrl_state *states;
	/** Number of state configurations. */
	uint8_t state_cnt;
};

/** Utility macro to indicate no register is used. */
#define PINCTRL_REG_NONE 0U

/** @cond INTERNAL_HIDDEN */

#ifndef CONFIG_PM_DEVICE
/** If device power management is not enabled, "sleep" state will be ignored. */
#define PINCTRL_SKIP_SLEEP 1
#endif

/**
 * @brief Obtain the state identifier for the given node and state index.
 *
 * @param state_idx State index.
 * @param node_id Node identifier.
 */
#define Z_PINCTRL_STATE_ID(state_idx, node_id)      \
    Z_CONCAT(PINCTRL_STATE_,                        \
        DT_PINCTRL_IDX_TO_NAME_UPPER_TOKEN(node_id, state_idx))

/**
 * @brief Obtain the variable name storing pinctrl config for the given DT node
 * identifier.
 *
 * @param node_id Node identifier.
 */
#define Z_PINCTRL_DEV_CONFIG_NAME(node_id) \
    Z_CONCAT(__pinctrl_dev_config, DEVICE_DT_NAME_GET(node_id))

/**
 * @brief Obtain the variable name storing pinctrl states for the given DT node
 * identifier.
 *
 * @param node_id Node identifier.
 */
#define Z_PINCTRL_STATES_NAME(node_id) \
    Z_CONCAT(__pinctrl_states, DEVICE_DT_NAME_GET(node_id))

/**
 * @brief Obtain the variable name storing pinctrl pins for the given DT node
 * identifier and state index.
 *
 * @param state_idx State index.
 * @param node_id Node identifier.
 */
#define Z_PINCTRL_STATE_PINS_NAME(state_idx, node_id) \
    Z_CONCAT(__pinctrl_state_pins_ ## state_idx, DEVICE_DT_NAME_GET(node_id))

/**
 * @brief Utility macro to check if given state has to be skipped.
 *
 * If a certain state has to be skipped, a macro named PINCTRL_SKIP_<STATE>
 * can be defined evaluating to 1. This can be useful, for example, to
 * automatically ignore the sleep state if no device power management is
 * enabled.
 *
 * @param state_idx State index.
 * @param node_id Node identifier.
 */
#define Z_PINCTRL_SKIP_STATE(state_idx, node_id)    \
    Z_CONCAT(PINCTRL_SKIP_,                         \
        DT_PINCTRL_IDX_TO_NAME_UPPER_TOKEN(node_id, state_idx))

/**
 * @brief Helper macro to define pins for a given pin control state.
 *
 * @param state_idx State index.
 * @param node_id Node identifier.
 */
#define Z_PINCTRL_STATE_PINS_DEFINE(state_idx, node_id)			       \
	COND_CODE_1(Z_PINCTRL_SKIP_STATE(state_idx, node_id), (),	       \
	(static const pinctrl_soc_pin_t					       \
	Z_PINCTRL_STATE_PINS_NAME(state_idx, node_id)[] =		       \
	Z_PINCTRL_STATE_PINS_INIT(node_id, pinctrl_ ## state_idx)))

/**
 * @brief Helper macro to initialize a pin control state.
 *
 * @param state_idx State index.
 * @param node_id Node identifier.
 */
#define Z_PINCTRL_STATE_INIT(state_idx, node_id)			       \
	COND_CODE_1(Z_PINCTRL_SKIP_STATE(state_idx, node_id), (),	       \
	({								       \
		.id = Z_PINCTRL_STATE_ID(state_idx, node_id),		       \
		.pins = Z_PINCTRL_STATE_PINS_NAME(state_idx, node_id),	       \
		.pin_cnt = ARRAY_SIZE(Z_PINCTRL_STATE_PINS_NAME(state_idx,     \
								node_id))      \
	}))

/**
 * @brief Define all the states for the given node identifier.
 *
 * @param node_id Node identifier.
 */
#define Z_PINCTRL_STATES_DEFINE(node_id)				       \
	static const struct pinctrl_state				       \
	Z_PINCTRL_STATES_NAME(node_id)[] = {				       \
		LISTIFY(DT_NUM_PINCTRL_STATES(node_id),			       \
			     Z_PINCTRL_STATE_INIT, (,), node_id)	       \
	};

#ifdef CONFIG_PINCTRL_STORE_REG
/**
 * @brief Helper macro to initialize pin control config.
 *
 * @param node_id Node identifier.
 */
#define Z_PINCTRL_DEV_CONFIG_INIT(node_id)				       \
	{								       \
		.reg = DT_REG_ADDR(node_id),				       \
		.states = Z_PINCTRL_STATES_NAME(node_id),		       \
		.state_cnt = ARRAY_SIZE(Z_PINCTRL_STATES_NAME(node_id)),       \
	}
#else
#define Z_PINCTRL_DEV_CONFIG_INIT(node_id)				       \
	{								       \
		.states = Z_PINCTRL_STATES_NAME(node_id),		       \
		.state_cnt = ARRAY_SIZE(Z_PINCTRL_STATES_NAME(node_id)),       \
	}
#endif

#ifdef CONFIG_PINCTRL_NON_STATIC
#define Z_PINCTRL_DEV_CONFIG_STATIC
#else
#define Z_PINCTRL_DEV_CONFIG_STATIC static
#endif

#ifdef CONFIG_PINCTRL_DYNAMIC
#define Z_PINCTRL_DEV_CONFIG_CONST
#else
#define Z_PINCTRL_DEV_CONFIG_CONST const
#endif

/** @endcond */

#if defined(CONFIG_PINCTRL_NON_STATIC) || defined(__DOXYGEN__)
/**
 * @brief Declare pin control configuration for a given node identifier.
 *
 * This macro should be used by tests or applications using runtime pin control
 * to declare the pin control configuration for a device.
 * #PINCTRL_DT_DEV_CONFIG_GET can later be used to obtain a reference to such
 * configuration.
 *
 * Only available if @kconfig{CONFIG_PINCTRL_NON_STATIC} is selected.
 *
 * @param node_id Node identifier.
 */
#define PINCTRL_DT_DEV_CONFIG_DECLARE(node_id)				       \
	extern Z_PINCTRL_DEV_CONFIG_CONST struct pinctrl_dev_config	       \
	Z_PINCTRL_DEV_CONFIG_NAME(node_id)
#endif /* defined(CONFIG_PINCTRL_NON_STATIC) || defined(__DOXYGEN__) */

/**
 * @brief Define all pin control information for the given node identifier.
 *
 * This helper macro should be called together with device definition. It
 * defines and initializes the pin control configuration for the device
 * represented by node_id. Each pin control state (pinctrl-0, ..., pinctrl-N) is
 * also defined and initialized. Note that states marked to be skipped will not
 * be defined (refer to Z_PINCTRL_SKIP_STATE for more details).
 *
 * @param node_id Node identifier.
 */
#define PINCTRL_DT_DEFINE(node_id)					       \
	LISTIFY(DT_NUM_PINCTRL_STATES(node_id),				       \
		     Z_PINCTRL_STATE_PINS_DEFINE, (;), node_id);	       \
	Z_PINCTRL_STATES_DEFINE(node_id)				       \
	Z_PINCTRL_DEV_CONFIG_STATIC Z_PINCTRL_DEV_CONFIG_CONST		       \
	struct pinctrl_dev_config Z_PINCTRL_DEV_CONFIG_NAME(node_id) =	       \
	Z_PINCTRL_DEV_CONFIG_INIT(node_id)

/**
 * @brief Define all pin control information for the given compatible index.
 *
 * @param inst Instance number.
 *
 * @see #PINCTRL_DT_DEFINE
 */
#define PINCTRL_DT_INST_DEFINE(inst) PINCTRL_DT_DEFINE(DT_DRV_INST(inst))

/**
 * @brief Obtain a reference to the pin control configuration given a node
 * identifier.
 *
 * @param node_id Node identifier.
 */
#define PINCTRL_DT_DEV_CONFIG_GET(node_id) &Z_PINCTRL_DEV_CONFIG_NAME(node_id)

/**
 * @brief Obtain a reference to the pin control configuration given current
 * compatible instance number.
 *
 * @param inst Instance number.
 *
 * @see #PINCTRL_DT_DEV_CONFIG_GET
 */
#define PINCTRL_DT_INST_DEV_CONFIG_GET(inst) \
	PINCTRL_DT_DEV_CONFIG_GET(DT_DRV_INST(inst))

#ifdef __cplusplus
}
#endif

/**
 * @}
 */
#if defined(CONFIG_ARCH_CHIP_S32K14X)
#include <zephyr/drivers/s32k1xx/s32k1xx_pinctrl.h>
#endif

#if defined(CONFIG_ARCH_CHIP_STM32H7)
#include <zephyr/drivers/stm32h7/stm32h7_pinctrl.h>
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_PINCTRL_H_ */
