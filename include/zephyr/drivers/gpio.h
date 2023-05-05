/**
 * @file
 * @brief Public APIs for GPIO drivers (NuttX Bindings)
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_GPIO_H_
#define ZEPHYR_INCLUDE_DRIVERS_GPIO_H_

#include <errno.h>
#include <nuttx/nuttx.h>
#include <nuttx/config.h>
#include <nuttx/board.h>
#include <zephyr/sys/util.h>

/**
 * @brief GPIO Driver APIs
 * @defgroup gpio_interface GPIO Driver APIs
 * @ingroup io_interfaces
 * @{
 */

/**
 * @name GPIO input/output configuration flags
 * @{
 */

/** Enables pin as input. */
#define GPIO_INPUT              (1U << 16)

/** Enables pin as output, no change to the output state. */
#define GPIO_OUTPUT             (1U << 17)

/** Disables pin for both input and output. */
#define GPIO_DISCONNECTED       0

/** @cond INTERNAL_HIDDEN */
/* Initializes output to a low state. */
#define GPIO_OUTPUT_INIT_LOW    (1U << 18)

/* Initializes output to a high state. */
#define GPIO_OUTPUT_INIT_HIGH   (1U << 19)

/* Initializes output based on logic level */
#define GPIO_OUTPUT_INIT_LOGICAL    (1U << 20)

/** @endcond */

/** Configures GPIO pin as output and initializes it to a low state. */
#define GPIO_OUTPUT_LOW         (GPIO_OUTPUT | GPIO_OUTPUT_INIT_LOW)
/** Configures GPIO pin as output and initializes it to a high state. */
#define GPIO_OUTPUT_HIGH        (GPIO_OUTPUT | GPIO_OUTPUT_INIT_HIGH)
/** Configures GPIO pin as output and initializes it to a logic 0. */
#define GPIO_OUTPUT_INACTIVE    (GPIO_OUTPUT | GPIO_OUTPUT_INIT_LOW | GPIO_OUTPUT_INIT_LOGICAL)
/** Configures GPIO pin as output and initializes it to a logic 1. */
#define GPIO_OUTPUT_ACTIVE      (GPIO_OUTPUT | GPIO_OUTPUT_INIT_HIGH | GPIO_OUTPUT_INIT_LOGICAL)

/** @} */

/**
 * @name GPIO interrupt configuration flags
 * The `GPIO_INT_*` flags are used to specify how input GPIO pins will trigger
 * interrupts. The interrupts can be sensitive to pin physical or logical level.
 * Interrupts sensitive to pin logical level take into account GPIO_ACTIVE_LOW
 * flag. If a pin was configured as Active Low, physical level low will be
 * considered as logical level 1 (an active state), physical level high will
 * be considered as logical level 0 (an inactive state).
 * @{
 */

/** Disables GPIO pin interrupt. */
#define GPIO_INT_DISABLE        (1U << 21)

/** @cond INTERNAL_HIDDEN */

/* Enables GPIO pin interrupt. */
#define GPIO_INT_ENABLE         (1U << 22)

/* GPIO interrupt is sensitive to logical levels.
 *
 * This is a component flag that should be combined with other
 * `GPIO_INT_*` flags to produce a meaningful configuration.
 */
#define GPIO_INT_LEVELS_LOGICAL (1U << 23)

/* GPIO interrupt is edge sensitive.
 *
 * Note: by default interrupts are level sensitive.
 *
 * This is a component flag that should be combined with other
 * `GPIO_INT_*` flags to produce a meaningful configuration.
 */
#define GPIO_INT_EDGE           (1U << 24)

/* Trigger detection when input state is (or transitions to) physical low or
 * logical 0 level.
 *
 * This is a component flag that should be combined with other
 * `GPIO_INT_*` flags to produce a meaningful configuration.
 */
#define GPIO_INT_LOW_0          (1U << 25)

/* Trigger detection on input state is (or transitions to) physical high or
 * logical 1 level.
 *
 * This is a component flag that should be combined with other
 * `GPIO_INT_*` flags to produce a meaningful configuration.
 */
#define GPIO_INT_HIGH_1         (1U << 26)

#define GPIO_INT_MASK           \
    (GPIO_INT_DISABLE | GPIO_INT_ENABLE | GPIO_INT_LEVELS_LOGICAL | GPIO_INT_EDGE | GPIO_INT_LOW_0 | GPIO_INT_HIGH_1)

/** @endcond */

/** Configures GPIO interrupt to be triggered on pin rising edge and enables it.
 */
#define GPIO_INT_EDGE_RISING    (GPIO_INT_ENABLE | GPIO_INT_EDGE | GPIO_INT_HIGH_1)

/** Configures GPIO interrupt to be triggered on pin falling edge and enables
 * it.
 */
#define GPIO_INT_EDGE_FALLING   (GPIO_INT_ENABLE | GPIO_INT_EDGE | GPIO_INT_LOW_0)

/** Configures GPIO interrupt to be triggered on pin rising or falling edge and
 * enables it.
 */
#define GPIO_INT_EDGE_BOTH      (GPIO_INT_ENABLE | GPIO_INT_EDGE | GPIO_INT_LOW_0 | GPIO_INT_HIGH_1)

/** Configures GPIO interrupt to be triggered on pin physical level low and
 * enables it.
 */
#define GPIO_INT_LEVEL_LOW      (GPIO_INT_ENABLE | GPIO_INT_LOW_0)

/** Configures GPIO interrupt to be triggered on pin physical level high and
 * enables it.
 */
#define GPIO_INT_LEVEL_HIGH     (GPIO_INT_ENABLE | GPIO_INT_HIGH_1)

/** Configures GPIO interrupt to be triggered on pin state change to logical
 * level 0 and enables it.
 */
#define GPIO_INT_EDGE_TO_INACTIVE (GPIO_INT_ENABLE | GPIO_INT_LEVELS_LOGICAL | GPIO_INT_EDGE | GPIO_INT_LOW_0)

/** Configures GPIO interrupt to be triggered on pin state change to logical
 * level 1 and enables it.
 */
#define GPIO_INT_EDGE_TO_ACTIVE (GPIO_INT_ENABLE | GPIO_INT_LEVELS_LOGICAL | GPIO_INT_EDGE | GPIO_INT_HIGH_1)

/** Configures GPIO interrupt to be triggered on pin logical level 0 and enables
 * it.
 */
#define GPIO_INT_LEVEL_INACTIVE (GPIO_INT_ENABLE | GPIO_INT_LEVELS_LOGICAL | GPIO_INT_LOW_0)

/** Configures GPIO interrupt to be triggered on pin logical level 1 and enables
 * it.
 */
#define GPIO_INT_LEVEL_ACTIVE   (GPIO_INT_ENABLE | GPIO_INT_LEVELS_LOGICAL | GPIO_INT_HIGH_1)

/** @} */

/** @cond INTERNAL_HIDDEN */
#define GPIO_DIR_MASK           (GPIO_INPUT | GPIO_OUTPUT)
/** @endcond */

/**
 * @brief Identifies a set of pins associated with a port.
 *
 * The pin with index n is present in the set if and only if the bit
 * identified by (1U << n) is set.
 */
typedef uint32_t gpio_port_pins_t;

/**
 * @brief Provides values for a set of pins associated with a port.
 *
 * The value for a pin with index n is high (physical mode) or active
 * (logical mode) if and only if the bit identified by (1U << n) is set.
 * Otherwise the value for the pin is low (physical mode) or inactive
 * (logical mode).
 *
 * Values of this type are often paired with a `gpio_port_pins_t` value
 * that specifies which encoded pin values are valid for the operation.
 */
typedef uint32_t gpio_port_value_t;

/**
 * @brief Provides a type to hold a GPIO pin index.
 *
 * This reduced-size type is sufficient to record a pin number,
 * e.g. from a devicetree GPIOS property.
 */
typedef uint8_t gpio_pin_t;

/**
 * @brief Provides a type to hold GPIO devicetree flags.
 *
 * All GPIO flags that can be expressed in devicetree fit in the low 16
 * bits of the full flags field, so use a reduced-size type to record
 * that part of a GPIOS property.
 *
 * The lower 8 bits are used for standard flags. The upper 8 bits are reserved
 * for SoC specific flags.
 */
typedef uint16_t gpio_dt_flags_t;

/**
 * @brief Provides a type to hold GPIO configuration flags.
 *
 * This type is sufficient to hold all flags used to control GPIO
 * configuration, whether pin or interrupt.
 */
typedef uint32_t gpio_flags_t;

/**
 * @brief Container for GPIO pin information specified in devicetree
 *
 * This type contains a pointer to a GPIO device, pin number for a pin
 * controlled by that device, and the subset of pin configuration
 * flags which may be given in devicetree.
 *
 * @see GPIO_DT_SPEC_GET_BY_IDX
 * @see GPIO_DT_SPEC_GET_BY_IDX_OR
 * @see GPIO_DT_SPEC_GET
 * @see GPIO_DT_SPEC_GET_OR
 */
struct gpio_dt_spec {
    /** GPIO device controlling the pin */
    const struct device* port;

	/** The pin's number on the device */
	gpio_pin_t pin;

    /** GPIO pin setting */
    uint32_t pinset;
};

struct gpio_callback;

typedef void (*gpio_callback_handler_t)(int irq, FAR void* context, FAR void* arg);

/**
 * @brief GPIO callback structure
 *
 * Used to register a callback in the driver instance callback list.
 * As many callbacks as needed can be added as long as each of them
 * are unique pointers of struct gpio_callback.
 * Beware such structure should not be allocated on stack.
 *
 * Note: To help setting it, see gpio_init_callback() below
 */
struct gpio_callback {
    /** Actual callback function being called when relevant. */
    gpio_callback_handler_t handler;

    /** A mask of pins the callback is interested in, if 0 the callback
     * will never be called. Such pin_mask can be modified whenever
     * necessary by the owner, and thus will affect the handler being
     * called or not. The selected pins must be configured to trigger
     * an interrupt.
     */
    gpio_port_pins_t pin_mask;
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * For internal use only, skip these in public documentation.
 */

/* Used by driver api function pin_interrupt_configure, these are defined
 * in terms of the public flags so we can just mask and pass them
 * through to the driver api
 */
enum gpio_int_mode {
    GPIO_INT_MODE_DISABLED = GPIO_INT_DISABLE,
    GPIO_INT_MODE_LEVEL    = GPIO_INT_ENABLE,
    GPIO_INT_MODE_EDGE     = GPIO_INT_ENABLE | GPIO_INT_EDGE,
};

enum gpio_int_trig {
    /* Trigger detection when input state is (or transitions to)
     * physical low. (Edge Failing or Active Low) */
    GPIO_INT_TRIG_LOW = GPIO_INT_LOW_0,
    /* Trigger detection when input state is (or transitions to)
     * physical high. (Edge Rising or Active High) */
    GPIO_INT_TRIG_HIGH = GPIO_INT_HIGH_1,
    /* Trigger detection on pin rising or falling edge. */
    GPIO_INT_TRIG_BOTH = GPIO_INT_LOW_0 | GPIO_INT_HIGH_1,
};

/**
 * @endcond
 */

#if defined(CONFIG_ARCH_CHIP_S32K14X)
#include <zephyr/drivers/s32k1xx/s32k1xx_gpio.h>
#endif

#if defined(CONFIG_ARCH_CHIP_STM32H7)
#include <zephyr/drivers/stm32h7/stm32h7_gpio.h>
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_GPIO_H_ */
