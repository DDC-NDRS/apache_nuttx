/*
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2020-2021 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public PWM Driver APIs
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PWM_H_
#define ZEPHYR_INCLUDE_DRIVERS_PWM_H_

/**
 * @brief PWM Interface
 * @defgroup pwm_interface PWM Interface
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name PWM capture configuration flags
 * @anchor PWM_CAPTURE_FLAGS
 * @{
 */

/** @cond INTERNAL_HIDDEN */
/* Bit 0 is used for PWM_POLARITY_NORMAL/PWM_POLARITY_INVERTED */
#define PWM_CAPTURE_TYPE_SHIFT		1U
#define PWM_CAPTURE_TYPE_MASK		(3U << PWM_CAPTURE_TYPE_SHIFT)
#define PWM_CAPTURE_MODE_SHIFT		3U
#define PWM_CAPTURE_MODE_MASK		(1U << PWM_CAPTURE_MODE_SHIFT)
/** @endcond */

/** PWM pin capture captures period. */
#define PWM_CAPTURE_TYPE_PERIOD		(1U << PWM_CAPTURE_TYPE_SHIFT)

/** PWM pin capture captures pulse width. */
#define PWM_CAPTURE_TYPE_PULSE		(2U << PWM_CAPTURE_TYPE_SHIFT)

/** PWM pin capture captures both period and pulse width. */
#define PWM_CAPTURE_TYPE_BOTH		(PWM_CAPTURE_TYPE_PERIOD | \
					 PWM_CAPTURE_TYPE_PULSE)

/** PWM pin capture captures a single period/pulse width. */
#define PWM_CAPTURE_MODE_SINGLE		(0U << PWM_CAPTURE_MODE_SHIFT)

/** PWM pin capture captures period/pulse width continuously. */
#define PWM_CAPTURE_MODE_CONTINUOUS	(1U << PWM_CAPTURE_MODE_SHIFT)

/** @} */

/**
 * @brief Provides a type to hold PWM configuration flags.
 *
 * The lower 8 bits are used for standard flags.
 * The upper 8 bits are reserved for SoC specific flags.
 *
 * @see @ref PWM_CAPTURE_FLAGS.
 */

typedef uint16_t pwm_flags_t;

/**
 * @brief Container for PWM information specified in devicetree.
 *
 * This type contains a pointer to a PWM device, channel number (controlled by
 * the PWM device), the PWM signal period in nanoseconds and the flags
 * applicable to the channel. Note that not all PWM drivers support flags. In
 * such case, flags will be set to 0.
 *
 * @see PWM_DT_SPEC_GET_BY_NAME
 * @see PWM_DT_SPEC_GET_BY_NAME_OR
 * @see PWM_DT_SPEC_GET_BY_IDX
 * @see PWM_DT_SPEC_GET_BY_IDX_OR
 * @see PWM_DT_SPEC_GET
 * @see PWM_DT_SPEC_GET_OR
 */
struct pwm_dt_spec {
	/** PWM device instance. */
	const struct device *dev;
	/** Channel number. */
	uint32_t channel;
	/** Period in nanoseconds. */
	uint32_t period;
	/** Flags. */
	pwm_flags_t flags;
};

/**
 * @brief PWM capture callback handler function signature
 *
 * @note The callback handler will be called in interrupt context.
 *
 * @note @kconfig{CONFIG_PWM_CAPTURE} must be selected to enable PWM capture
 * support.
 *
 * @param[in] dev PWM device instance.
 * @param channel PWM channel.

 * @param period_cycles Captured PWM period width (in clock cycles). HW
 *                      specific.
 * @param pulse_cycles Captured PWM pulse width (in clock cycles). HW specific.
 * @param status Status for the PWM capture (0 if no error, negative errno
 *               otherwise. See pwm_capture_cycles() return value
 *               descriptions for details).
 * @param user_data User data passed to pwm_configure_capture()
 */
typedef void (*pwm_capture_callback_handler_t)(const struct device* dev,
					                           uint32_t channel,
					                           uint32_t period_cycles,
					                           uint32_t pulse_cycles,
					                           int status, void* user_data);

/** @cond INTERNAL_HIDDEN */
/**
 * @brief PWM driver API call to configure PWM pin period and pulse width.
 * @see pwm_set_cycles() for argument description.
 */
typedef int (*pwm_set_cycles_t)(const struct device* dev, uint32_t channel,
				                uint32_t period_cycles, uint32_t pulse_cycles,
				                pwm_flags_t flags);

/**
 * @brief PWM driver API call to obtain the PWM cycles per second (frequency).
 * @see pwm_get_cycles_per_sec() for argument description
 */
typedef int (*pwm_get_cycles_per_sec_t)(const struct device* dev,
					                    uint32_t channel, uint64_t *cycles);

/**
 * @brief Set the period and pulse width for a single PWM output.
 *
 * The PWM period and pulse width will synchronously be set to the new values
 * without glitches in the PWM signal, but the call will not block for the
 * change to take effect.
 *
 * @note Not all PWM controllers support synchronous, glitch-free updates of the
 * PWM period and pulse width. Depending on the hardware, changing the PWM
 * period and/or pulse width may cause a glitch in the generated PWM signal.
 *
 * @note Some multi-channel PWM controllers share the PWM period across all
 * channels. Depending on the hardware, changing the PWM period for one channel
 * may affect the PWM period for the other channels of the same PWM controller.
 *
 * Passing 0 as @p pulse will cause the pin to be driven to a constant
 * inactive level.
 * Passing a non-zero @p pulse equal to @p period will cause the pin
 * to be driven to a constant active level.
 *
 * @param[in] dev PWM device instance.
 * @param channel PWM channel.
 * @param period Period (in clock cycles) set to the PWM. HW specific.
 * @param pulse Pulse width (in clock cycles) set to the PWM. HW specific.
 * @param flags Flags for pin configuration.
 *
 * @retval 0 If successful.
 * @retval -EINVAL If pulse > period.
 * @retval -errno Negative errno code on failure.
 */
static inline int pwm_set_cycles(const struct device* dev, uint32_t channel,
                                 uint32_t period, uint32_t pulse,
                                 pwm_flags_t flags) {
    return (-EINVAL);
}

/**
 * @brief Get the clock rate (cycles per second) for a single PWM output.
 *
 * @param[in] dev PWM device instance.
 * @param channel PWM channel.
 * @param[out] cycles Pointer to the memory to store clock rate (cycles per
 *                    sec). HW specific.
 *
 * @retval 0 If successful.
 * @retval -errno Negative errno code on failure.
 */
static inline int pwm_get_cycles_per_sec(const struct device *dev, uint32_t channel,
				                         uint64_t *cycles) {
    return (-EINVAL);
}

/**
 * @brief Set the period and pulse width in nanoseconds for a single PWM output.
 *
 * @note Utility macros such as PWM_MSEC() can be used to convert from other
 * scales or units to nanoseconds, the units used by this function.
 *
 * @param[in] dev PWM device instance.
 * @param channel PWM channel.
 * @param period Period (in nanoseconds) set to the PWM.
 * @param pulse Pulse width (in nanoseconds) set to the PWM.
 * @param flags Flags for pin configuration (polarity).
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP If requested period or pulse cycles are not supported.
 * @retval -errno Other negative errno code on failure.
 */
static inline int pwm_set(const struct device *dev, uint32_t channel,
			              uint32_t period, uint32_t pulse, pwm_flags_t flags) {
	int err;
	uint64_t pulse_cycles;
	uint64_t period_cycles;
	uint64_t cycles_per_sec;

	err = pwm_get_cycles_per_sec(dev, channel, &cycles_per_sec);
	if (err < 0) {
		return err;
	}

	period_cycles = (period * cycles_per_sec) / NSEC_PER_SEC;
	if (period_cycles > UINT32_MAX) {
		return -ENOTSUP;
	}

	pulse_cycles = (pulse * cycles_per_sec) / NSEC_PER_SEC;
	if (pulse_cycles > UINT32_MAX) {
		return -ENOTSUP;
	}

    return pwm_set_cycles(dev, channel, (uint32_t)period_cycles, (uint32_t)pulse_cycles, flags);
}

/**
 * @brief Set the period and pulse width in nanoseconds from a struct
 *        pwm_dt_spec (with custom period).
 *
 * This is equivalent to:
 *
 *     pwm_set(spec->dev, spec->channel, period, pulse, spec->flags)
 *
 * The period specified in @p spec is ignored. This API call can be used when
 * the period specified in Devicetree needs to be changed at runtime.
 *
 * @param[in] spec PWM specification from devicetree.
 * @param period Period (in nanoseconds) set to the PWM.
 * @param pulse Pulse width (in nanoseconds) set to the PWM.
 *
 * @return A value from pwm_set().
 *
 * @see pwm_set_pulse_dt()
 */
static inline int pwm_set_dt(const struct pwm_dt_spec* spec, uint32_t period, uint32_t pulse) {
	return pwm_set(spec->dev, spec->channel, period, pulse, spec->flags);
}

/**
 * @brief Set the period and pulse width in nanoseconds from a struct
 *        pwm_dt_spec.
 *
 * This is equivalent to:
 *
 *     pwm_set(spec->dev, spec->channel, spec->period, pulse, spec->flags)
 *
 * @param[in] spec PWM specification from devicetree.
 * @param pulse Pulse width (in nanoseconds) set to the PWM.
 *
 * @return A value from pwm_set().
 *
 * @see pwm_set_pulse_dt()
 */
static inline int pwm_set_pulse_dt(const struct pwm_dt_spec* spec, uint32_t pulse) {
    return pwm_set(spec->dev, spec->channel, spec->period, pulse, spec->flags);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */


#endif /* ZEPHYR_INCLUDE_DRIVERS_PWM_H_ */
