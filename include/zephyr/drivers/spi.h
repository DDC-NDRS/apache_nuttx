/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for SPI drivers and applications
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SPI_H_
#define ZEPHYR_INCLUDE_DRIVERS_SPI_H_

/**
 * @brief SPI Interface
 * @defgroup spi_interface SPI Interface
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name SPI operational mode
 * @{
 */
#define SPI_OP_MODE_MASTER	0U
#define SPI_OP_MODE_SLAVE	BIT(0)
#define SPI_OP_MODE_MASK	0x1U
#define SPI_OP_MODE_GET(_operation_) ((_operation_) & SPI_OP_MODE_MASK)
/** @} */

/**
 * @name SPI Polarity & Phase Modes
 * @{
 */

/**
 * Clock Polarity: if set, clock idle state will be 1
 * and active state will be 0. If untouched, the inverse will be true
 * which is the default.
 */
#define SPI_MODE_CPOL		BIT(1)

/**
 * Clock Phase: this dictates when is the data captured, and depends
 * clock's polarity. When SPI_MODE_CPOL is set and this bit as well,
 * capture will occur on low to high transition and high to low if
 * this bit is not set (default). This is fully reversed if CPOL is
 * not set.
 */
#define SPI_MODE_CPHA		BIT(2)

/**
 * Whatever data is transmitted is looped-back to the receiving buffer of
 * the controller. This is fully controller dependent as some may not
 * support this, and can be used for testing purposes only.
 */
#define SPI_MODE_LOOP		BIT(3)

#define SPI_MODE_MASK		(0xEU)
#define SPI_MODE_GET(_mode_)			\
	((_mode_) & SPI_MODE_MASK)

/** @} */

/**
 * @name SPI Transfer modes (host controller dependent)
 * @{
 */
#define SPI_TRANSFER_MSB	(0U)
#define SPI_TRANSFER_LSB	BIT(4)
/** @} */

/**
 * @name SPI word size
 * @{
 */
#define SPI_WORD_SIZE_SHIFT	(5U)
#define SPI_WORD_SIZE_MASK	(0x3FU << SPI_WORD_SIZE_SHIFT)
#define SPI_WORD_SIZE_GET(_operation_)					\
	(((_operation_) & SPI_WORD_SIZE_MASK) >> SPI_WORD_SIZE_SHIFT)

#define SPI_WORD_SET(_word_size_)		\
	((_word_size_) << SPI_WORD_SIZE_SHIFT)
/** @} */

/**
 * @name Specific SPI devices control bits
 * @{
 */
/* Requests - if possible - to keep CS asserted after the transaction */
#define SPI_HOLD_ON_CS		BIT(12)
/* Keep the device locked after the transaction for the current config.
 * Use this with extreme caution (see spi_release() below) as it will
 * prevent other callers to access the SPI device until spi_release() is
 * properly called.
 */
#define SPI_LOCK_ON		BIT(13)

/* Active high logic on CS - Usually, and by default, CS logic is active
 * low. However, some devices may require the reverse logic: active high.
 * This bit will request the controller to use that logic. Note that not
 * all controllers are able to handle that natively. In this case deferring
 * the CS control to a gpio line through struct spi_cs_control would be
 * the solution.
 */
#define SPI_CS_ACTIVE_HIGH	BIT(14)
/** @} */

/**
 * @name SPI MISO lines (if @kconfig{CONFIG_SPI_EXTENDED_MODES} is enabled)
 * @{
 *
 * Some controllers support dual, quad or octal MISO lines connected to slaves.
 * Default is single, which is the case most of the time.
 * Without @kconfig{CONFIG_SPI_EXTENDED_MODES} being enabled, single is the
 * only supported one.
 */
#define SPI_LINES_SINGLE	(0U << 16)
#define SPI_LINES_DUAL		(1U << 16)
#define SPI_LINES_QUAD		(2U << 16)
#define SPI_LINES_OCTAL		(3U << 16)

#define SPI_LINES_MASK		(0x3U << 16)
/** @} */

/**
 * @brief SPI Chip Select control structure
 *
 * This can be used to control a CS line via a GPIO line, instead of
 * using the controller inner CS logic.
 *
 */
struct spi_cs_control {
	/**
	 * GPIO devicetree specification of CS GPIO.
	 * The device pointer can be set to NULL to fully inhibit CS control if
	 * necessary. The GPIO flags GPIO_ACTIVE_LOW/GPIO_ACTIVE_HIGH should be
	 * equivalent to SPI_CS_ACTIVE_HIGH/SPI_CS_ACTIVE_LOW options in struct
	 * spi_config.
	 */
	struct gpio_dt_spec gpio;

	/**
	 * Delay in microseconds to wait before starting the
	 * transmission and before releasing the CS line.
	 */
	uint32_t delay;
};

/**
 * @brief SPI controller configuration structure
 *
 * @param frequency is the bus frequency in Hertz
 * @param operation is a bit field with the following parts:
 *
 *     operational mode    [ 0 ]       - master or slave.
 *     mode                [ 1 : 3 ]   - Polarity, phase and loop mode.
 *     transfer            [ 4 ]       - LSB or MSB first.
 *     word_size           [ 5 : 10 ]  - Size of a data frame in bits.
 *     duplex              [ 11 ]      - full/half duplex.
 *     cs_hold             [ 12 ]      - Hold on the CS line if possible.
 *     lock_on             [ 13 ]      - Keep resource locked for the caller.
 *     cs_active_high      [ 14 ]      - Active high CS logic.
 *     format              [ 15 ]      - Motorola or TI frame format (optional).
 * if @kconfig{CONFIG_SPI_EXTENDED_MODES} is defined:
 *     lines               [ 16 : 17 ] - MISO lines: Single/Dual/Quad/Octal.
 *     reserved            [ 18 : 31 ] - reserved for future use.
 * @param slave is the slave number from 0 to host controller slave limit.
 * @param cs is a valid pointer on a struct spi_cs_control is CS line is
 *    emulated through a gpio line, or NULL otherwise.
 * @warning Most drivers use pointer comparison to determine whether a
 * passed configuration is different from one used in a previous
 * transaction.  Changes to fields in the structure may not be
 * detected.
 */
struct spi_config {
	uint32_t		frequency;
#if defined(CONFIG_SPI_EXTENDED_MODES)
	uint32_t		operation;
	uint16_t		slave;
	uint16_t		_unused;
#else
	uint16_t		operation;
	uint16_t		slave;
#endif /* CONFIG_SPI_EXTENDED_MODES */

	const struct spi_cs_control *cs;
};

/**
 * @brief Complete SPI DT information
 *
 * @param bus is the SPI bus
 * @param config is the slave specific configuration
 */
struct spi_dt_spec {
	const struct device *bus;
	struct spi_config config;
};

/**
 * @brief SPI buffer structure
 *
 * @param buf is a valid pointer on a data buffer, or NULL otherwise.
 * @param len is the length of the buffer or, if buf is NULL, will be the
 *    length which as to be sent as dummy bytes (as TX buffer) or
 *    the length of bytes that should be skipped (as RX buffer).
 */
struct spi_buf {
    void*  buf;
    size_t len;
};

/**
 * @brief SPI buffer array structure
 *
 * @param buffers is a valid pointer on an array of spi_buf, or NULL.
 * @param count is the length of the array pointed by buffers.
 */
struct spi_buf_set {
    const struct spi_buf* buffers;
    size_t count;
};

/**
 * @typedef spi_api_io
 * @brief Callback API for I/O
 * See spi_transceive() for argument descriptions
 */
typedef int (*spi_api_io)(const struct device *dev,
			  const struct spi_config *config,
			  const struct spi_buf_set *tx_bufs,
			  const struct spi_buf_set *rx_bufs);

/**
 * @brief SPI callback for asynchronous transfer requests
 *
 * @param dev SPI device which is notifying of transfer completion or error
 * @param result Result code of the transfer request. 0 is success, -errno for failure.
 * @param data Transfer requester supplied data which is passed along to the callback.
 */
typedef void (*spi_callback_t)(const struct device *dev, int result, void *data);

/**
 * @typedef spi_api_io
 * @brief Callback API for asynchronous I/O
 * See spi_transceive_async() for argument descriptions
 */
typedef int (*spi_api_io_async)(const struct device *dev,
				const struct spi_config *config,
				const struct spi_buf_set *tx_bufs,
				const struct spi_buf_set *rx_bufs,
				spi_callback_t cb,
				void *userdata);

/**
 * @typedef spi_api_release
 * @brief Callback API for unlocking SPI device.
 * See spi_release() for argument descriptions
 */
typedef int (*spi_api_release)(const struct device *dev,
			       const struct spi_config *config);


/**
 * @brief SPI driver API
 * This is the mandatory API any SPI driver needs to expose.
 */
struct spi_driver_api {
	spi_api_io transceive;
#ifdef CONFIG_SPI_ASYNC
	spi_api_io_async transceive_async;
#endif /* CONFIG_SPI_ASYNC */
	spi_api_release release;
};

/**
 * @brief Validate that SPI bus (and CS gpio if defined) is ready.
 *
 * @param spec SPI specification from devicetree
 *
 * @retval true if the SPI bus is ready for use.
 * @retval false if the SPI bus (or the CS gpio defined) is not ready for use.
 */
static inline bool spi_is_ready_dt(const struct spi_dt_spec* spec) {
    return (true);
}

/**
 * @brief Read/write the specified amount of data from the SPI driver.
 *
 * @note This function is synchronous.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param config Pointer to a valid spi_config structure instance.
 *        Pointer-comparison may be used to detect changes from
 *        previous operations.
 * @param tx_bufs Buffer array where data to be sent originates from,
 *        or NULL if none.
 * @param rx_bufs Buffer array where data to be read will be written to,
 *        or NULL if none.
 *
 * @retval frames Positive number of frames received in slave mode.
 * @retval 0 If successful in master mode.
 * @retval -errno Negative errno code on failure.
 */
static inline int spi_transceive(const struct device* dev,
                                 const struct spi_config* config,
                                 const struct spi_buf_set* tx_bufs,
			                     const struct spi_buf_set* rx_bufs) {
    return (0);
}

/**
 * @brief Read/write data from an SPI bus specified in @p spi_dt_spec.
 *
 * This is equivalent to:
 *
 *     spi_transceive(spec->bus, &spec->config, tx_bufs, rx_bufs);
 *
 * @param spec SPI specification from devicetree
 * @param tx_bufs Buffer array where data to be sent originates from,
 *        or NULL if none.
 * @param rx_bufs Buffer array where data to be read will be written to,
 *        or NULL if none.
 *
 * @return a value from spi_transceive().
 */
static inline int spi_transceive_dt(const struct spi_dt_spec* spec,
                                    const struct spi_buf_set* tx_bufs,
                                    const struct spi_buf_set* rx_bufs) {
    return spi_transceive(spec->bus, &spec->config, tx_bufs, rx_bufs);
}

/**
 * @brief Read the specified amount of data from the SPI driver.
 *
 * @note This function is synchronous.
 *
 * @note This function is an helper function calling spi_transceive.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param config Pointer to a valid spi_config structure instance.
 *        Pointer-comparison may be used to detect changes from
 *        previous operations.
 * @param rx_bufs Buffer array where data to be read will be written to.
 *
 * @retval 0 If successful.
 * @retval -errno Negative errno code on failure.
 */
static inline int spi_read(const struct device* dev,
                           const struct spi_config* config,
                           const struct spi_buf_set* rx_bufs) {
    return spi_transceive(dev, config, NULL, rx_bufs);
}

/**
 * @brief Read data from a SPI bus specified in @p spi_dt_spec.
 *
 * This is equivalent to:
 *
 *     spi_read(spec->bus, &spec->config, rx_bufs);
 *
 * @param spec SPI specification from devicetree
 * @param rx_bufs Buffer array where data to be read will be written to.
 *
 * @return a value from spi_read().
 */
static inline int spi_read_dt(const struct spi_dt_spec* spec,
                              const struct spi_buf_set* rx_bufs) {
    return spi_read(spec->bus, &spec->config, rx_bufs);
}

/**
 * @brief Write the specified amount of data from the SPI driver.
 *
 * @note This function is synchronous.
 *
 * @note This function is an helper function calling spi_transceive.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param config Pointer to a valid spi_config structure instance.
 *        Pointer-comparison may be used to detect changes from
 *        previous operations.
 * @param tx_bufs Buffer array where data to be sent originates from.
 *
 * @retval 0 If successful.
 * @retval -errno Negative errno code on failure.
 */
static inline int spi_write(const struct device* dev,
                            const struct spi_config* config,
                            const struct spi_buf_set* tx_bufs) {
    return spi_transceive(dev, config, tx_bufs, NULL);
}

/**
 * @brief Write data to a SPI bus specified in @p spi_dt_spec.
 *
 * This is equivalent to:
 *
 *     spi_write(spec->bus, &spec->config, tx_bufs);
 *
 * @param spec SPI specification from devicetree
 * @param tx_bufs Buffer array where data to be sent originates from.
 *
 * @return a value from spi_write().
 */
static inline int spi_write_dt(const struct spi_dt_spec* spec,
                               const struct spi_buf_set* tx_bufs) {
    return spi_write(spec->bus, &spec->config, tx_bufs);
}

/**
 * @brief Release the SPI device specified in @p spi_dt_spec.
 *
 * This is equivalent to:
 *
 *     spi_release(spec->bus, &spec->config);
 *
 * @param spec SPI specification from devicetree
 *
 * @return a value from spi_release().
 */
static inline int spi_release_dt(const struct spi_dt_spec* spec) {
	return (0);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SPI_H_ */
