/**
 * @file
 * @brief Public APIs for GPIO drivers (NuttX Bindings)
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_STM32H7_GPIO_H_
#define ZEPHYR_INCLUDE_DRIVERS_STM32H7_GPIO_H_

#include <errno.h>
#include <nuttx/nuttx.h>
#include <nuttx/config.h>
#include <nuttx/board.h>
#include <arch/arm/src/stm32h7/stm32_gpio.h>

#include <zephyr/drivers/gpio.h>            // Re-include for struct gpio_dt_spec

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Validate that GPIO port is ready.
 *
 * @param spec GPIO specification from devicetree
 *
 * @retval true if the GPIO spec is ready for use.
 * @retval false if the GPIO spec is not ready for use.
 */
static inline bool gpio_is_ready_dt(const struct gpio_dt_spec* spec) {
    /* Validate port is ready */
    return (true);
}

static inline int gpio_pin_interrupt_configure_dt(const struct gpio_dt_spec* spec, gpio_flags_t flags) {
    /* Not compatible with Zephyr */
    return (0);
}

static inline int gpio_pin_configure_dt(const struct gpio_dt_spec* spec, gpio_flags_t extra_flags) {
    int ret;
    
    ret =  stm32_configgpio(spec->pinset);

    return (ret);
}

static inline int gpio_pin_get_dt(const struct gpio_dt_spec* spec) {
    bool val = stm32_gpioread(spec->pinset);

    return ((int)(val));
}

static inline int gpio_pin_set_dt(const struct gpio_dt_spec* spec, int value) {
    stm32_gpiowrite(spec->pinset, (bool)(value));

    return (0);
}

static inline int gpio_pin_set_raw(const struct device* port, gpio_pin_t pin, int value) {
    struct gpio_dt_spec* spec = container_of(port, struct gpio_dt_spec, port);
    uint32_t pinset = spec->pinset;
    
    stm32_gpiowrite(pinset, (bool)(value));

    return (0);
}

static inline int gpio_pin_toggle_dt(const struct gpio_dt_spec* spec) {
    bool val = stm32_gpioread(spec->pinset);
    stm32_gpiowrite(spec->pinset, (val ^ 1));

    return (0);
}

/**
 * @brief Helper to initialize a struct gpio_callback properly
 * @param callback A valid Application's callback structure pointer.
 * @param handler A valid handler function pointer.
 * @param pin_mask A bit mask of relevant pins for the handler
 */
static inline void gpio_init_callback(struct gpio_callback* callback, gpio_callback_handler_t handler,
                                      gpio_port_pins_t pin_mask) {
    callback->handler  = handler;
    callback->pin_mask = pin_mask;
}

/**
 * @brief Add an application callback.
 * @param port Pointer to the device structure for the driver instance.
 * @param callback A valid Application's callback structure pointer.
 * @return 0 if successful, negative errno code on failure.
 *
 * @note Callbacks may be added to the device from within a callback
 * handler invocation, but whether they are invoked for the current
 * GPIO event is not specified.
 *
 * Note: enables to add as many callback as needed on the same port.
 */
static inline int gpio_add_callback(const struct device* port, struct gpio_callback* callback) {
    struct gpio_dt_spec* spec = container_of(port, struct gpio_dt_spec, port);
    uint32_t pinset = spec->pinset;
    int ret;

    // Hard coded, to use falling edge and generate interrupt (not event)
    ret = stm32_gpiosetevent(pinset, false, true, false, (xcpt_t)callback->handler, NULL);

    return (ret);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_STM32H7_GPIO_H_ */
