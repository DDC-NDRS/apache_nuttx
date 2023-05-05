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
#ifndef ZEPHYR_INCLUDE_DRIVERS_STM32H7_UART_H_
#define ZEPHYR_INCLUDE_DRIVERS_STM32H7_UART_H_

/**
 * @brief UART Interface
 * @defgroup uart_interface UART Interface
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>
#include <stddef.h>

#include <nuttx/serial/serial.h>
#include <zephyr/drivers/uart.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Check whether an error was detected.
 *
 * @param dev UART device instance.
 *
 * @retval 0 If no error was detected.
 * @retval err Error flags as defined in @ref uart_rx_stop_reason
 * @retval -ENOSYS If not implemented.
 */
static inline int uart_err_check(void const* dev) {
    return (0);
}

/**
 * @defgroup uart_polling Polling UART API
 * @{
 */

/**
 * @brief Read a character from the device for input.
 *
 * This routine checks if the receiver has valid data.  When the
 * receiver has valid data, it reads a character from the device,
 * stores to the location pointed to by p_char, and returns 0 to the
 * calling thread. It returns -1, otherwise. This function is a
 * non-blocking call.
 *
 * @param dev UART device instance.
 * @param p_char Pointer to character.
 *
 * @retval 0 If a character arrived.
 * @retval -1 If no character was available to read (i.e. the UART
 *            input buffer was empty).
 * @retval -ENOSYS If the operation is not implemented.
 * @retval -EBUSY If async reception was enabled using @ref uart_rx_enable
 */
static inline int uart_poll_in(void const* dev, unsigned char* p_char) {
    return (-ENOSYS);
}

/**
 * @brief Read a 16-bit datum from the device for input.
 *
 * This routine checks if the receiver has valid data.  When the
 * receiver has valid data, it reads a 16-bit datum from the device,
 * stores to the location pointed to by p_u16, and returns 0 to the
 * calling thread. It returns -1, otherwise. This function is a
 * non-blocking call.
 *
 * @param dev UART device instance.
 * @param p_u16 Pointer to 16-bit data.
 *
 * @retval 0  If data arrived.
 * @retval -1 If no data was available to read (i.e., the UART
 *            input buffer was empty).
 * @retval -ENOTSUP If API is not enabled.
 * @retval -ENOSYS If the function is not implemented.
 * @retval -EBUSY If async reception was enabled using @ref uart_rx_enable
 */
static inline int uart_poll_in_u16(void const* dev, uint16_t* p_u16) {
    return (-ENOSYS);
}

/**
 * @brief Write a character to the device for output.
 *
 * This routine checks if the transmitter is full.  When the
 * transmitter is not full, it writes a character to the data
 * register. It waits and blocks the calling thread, otherwise. This
 * function is a blocking call.
 *
 * To send a character when hardware flow control is enabled, the handshake
 * signal CTS must be asserted.
 *
 * @param dev UART device instance.
 * @param out_char Character to send.
 */
static inline void uart_poll_out(void const* dev, unsigned char out_char) {
    // pass
}

/**
 * @brief Write a 16-bit datum to the device for output.
 *
 * This routine checks if the transmitter is full. When the
 * transmitter is not full, it writes a 16-bit datum to the data
 * register. It waits and blocks the calling thread, otherwise. This
 * function is a blocking call.
 *
 * To send a datum when hardware flow control is enabled, the handshake
 * signal CTS must be asserted.
 *
 * @param dev UART device instance.
 * @param out_u16 Wide data to send.
 */
static inline void uart_poll_out_u16(void const* dev, uint16_t out_u16) {
    // pass
}

/**
 * @}
 */

/**
 * @brief Set UART configuration.
 *
 * Sets UART configuration using data from *cfg.
 *
 * @param dev UART device instance.
 * @param cfg UART configuration structure.
 *
 * @retval 0 If successful.
 * @retval -errno Negative errno code in case of failure.
 * @retval -ENOSYS If configuration is not supported by device
 *                  or driver does not support setting configuration in runtime.
 */
static inline int uart_configure(void const* dev, const struct uart_config* cfg) {
    struct uart_dev_s* uart = (struct uart_dev_s*)dev;
    uart->zephyr.cfg = cfg;
    uart_setup(uart);
}

/**
 * @brief Get UART configuration.
 *
 * Stores current UART configuration to *cfg, can be used to retrieve initial
 * configuration after device was initialized using data from DTS.
 *
 * @param dev UART device instance.
 * @param cfg UART configuration structure.
 *
 * @retval 0 If successful.
 * @retval -errno Negative errno code in case of failure.
 * @retval -ENOSYS If driver does not support getting current configuration.
 */
static inline int uart_config_get(void const* dev, struct uart_config* cfg) {
    return (0);
}

/**
 * @addtogroup uart_interrupt
 * @{
 */

/**
 * @brief Fill FIFO with data.
 *
 * @details This function is expected to be called from UART
 * interrupt handler (ISR), if uart_irq_tx_ready() returns true.
 * Result of calling this function not from an ISR is undefined
 * (hardware-dependent). Likewise, *not* calling this function
 * from an ISR if uart_irq_tx_ready() returns true may lead to
 * undefined behavior, e.g. infinite interrupt loops. It's
 * mandatory to test return value of this function, as different
 * hardware has different FIFO depth (oftentimes just 1).
 *
 * @param dev UART device instance.
 * @param tx_data Data to transmit.
 * @param size Number of bytes to send.
 *
 * @return Number of bytes sent.
 * @retval -ENOSYS  if this function is not supported
 * @retval -ENOTSUP If API is not enabled.
 */
static inline int uart_fifo_fill(void const* dev,
                                 const uint8_t* tx_data, int size) {
    return (-ENOSYS);
}

/**
 * @brief Fill FIFO with wide data.
 *
 * @details This function is expected to be called from UART
 * interrupt handler (ISR), if uart_irq_tx_ready() returns true.
 * Result of calling this function not from an ISR is undefined
 * (hardware-dependent). Likewise, *not* calling this function
 * from an ISR if uart_irq_tx_ready() returns true may lead to
 * undefined behavior, e.g. infinite interrupt loops. It's
 * mandatory to test return value of this function, as different
 * hardware has different FIFO depth (oftentimes just 1).
 *
 * @param dev UART device instance.
 * @param tx_data Wide data to transmit.
 * @param size Number of datum to send.
 *
 * @return Number of datum sent.
 * @retval -ENOSYS If this function is not implemented
 * @retval -ENOTSUP If API is not enabled.
 */
static inline int uart_fifo_fill_u16(void const* dev,
                                     const uint16_t* tx_data, int size) {
    return (-ENOSYS);
}

/**
 * @brief Read data from FIFO.
 *
 * @details This function is expected to be called from UART
 * interrupt handler (ISR), if uart_irq_rx_ready() returns true.
 * Result of calling this function not from an ISR is undefined
 * (hardware-dependent). It's unspecified whether "RX ready"
 * condition as returned by uart_irq_rx_ready() is level- or
 * edge- triggered. That means that once uart_irq_rx_ready() is
 * detected, uart_fifo_read() must be called until it reads all
 * available data in the FIFO (i.e. until it returns less data
 * than was requested).
 *
 * Note that the calling context only applies to physical UARTs and
 * no to the virtual ones found in USB CDC ACM code.
 *
 * @param dev UART device instance.
 * @param rx_data Data container.
 * @param size Container size.
 *
 * @return Number of bytes read.
 * @retval -ENOSYS If this function is not implemented.
 * @retval -ENOTSUP If API is not enabled.
 */
static inline int uart_fifo_read(void const* dev, uint8_t* rx_data, const int size) {
    return (-ENOSYS);
}

/**
 * @brief Read wide data from FIFO.
 *
 * @details This function is expected to be called from UART
 * interrupt handler (ISR), if uart_irq_rx_ready() returns true.
 * Result of calling this function not from an ISR is undefined
 * (hardware-dependent). It's unspecified whether "RX ready"
 * condition as returned by uart_irq_rx_ready() is level- or
 * edge- triggered. That means that once uart_irq_rx_ready() is
 * detected, uart_fifo_read() must be called until it reads all
 * available data in the FIFO (i.e. until it returns less data
 * than was requested).
 *
 * Note that the calling context only applies to physical UARTs and
 * no to the virtual ones found in USB CDC ACM code.
 *
 * @param dev UART device instance.
 * @param rx_data Wide data container.
 * @param size Container size.
 *
 * @return Number of datum read.
 * @retval -ENOSYS If this function is not implemented.
 * @retval -ENOTSUP If API is not enabled.
 */
static inline int uart_fifo_read_u16(void const* dev,
                                     uint16_t* rx_data, const int size) {
    return (-ENOSYS);
}

/**
 * @brief Enable TX interrupt in IER.
 *
 * @param dev UART device instance.
 */
static inline void uart_irq_tx_enable(void const* dev) {
    // pass
}

/**
 * @brief Disable TX interrupt in IER.
 *
 * @param dev UART device instance.
 */
static inline void uart_irq_tx_disable(void const* dev) {
    // pass
}

/**
 * @brief Check if UART TX buffer can accept a new char
 *
 * @details Check if UART TX buffer can accept at least one character
 * for transmission (i.e. uart_fifo_fill() will succeed and return
 * non-zero). This function must be called in a UART interrupt
 * handler, or its result is undefined. Before calling this function
 * in the interrupt handler, uart_irq_update() must be called once per
 * the handler invocation.
 *
 * @param dev UART device instance.
 *
 * @retval 1 If TX interrupt is enabled and at least one char can be
 *           written to UART.
 * @retval 0 If device is not ready to write a new byte.
 * @retval -ENOSYS If this function is not implemented.
 * @retval -ENOTSUP If API is not enabled.
 */
static inline int uart_irq_tx_ready(void const* dev) {
    return (-ENOSYS);
}

/**
 * @brief Enable RX interrupt.
 *
 * @param dev UART device instance.
 */
static inline void uart_irq_rx_enable(void const* dev) {
    // pass
}

/**
 * @brief Disable RX interrupt.
 *
 * @param dev UART device instance.
 */
static inline void uart_irq_rx_disable(void const* dev) {
    // pass
}

/**
 * @brief Check if UART TX block finished transmission
 *
 * @details Check if any outgoing data buffered in UART TX block was
 * fully transmitted and TX block is idle. When this condition is
 * true, UART device (or whole system) can be power off. Note that
 * this function is *not* useful to check if UART TX can accept more
 * data, use uart_irq_tx_ready() for that. This function must be called
 * in a UART interrupt handler, or its result is undefined. Before
 * calling this function in the interrupt handler, uart_irq_update()
 * must be called once per the handler invocation.
 *
 * @param dev UART device instance.
 *
 * @retval 1 If nothing remains to be transmitted.
 * @retval 0 If transmission is not completed.
 * @retval -ENOSYS If this function is not implemented.
 * @retval -ENOTSUP If API is not enabled.
 */
static inline int uart_irq_tx_complete(void const* dev) {
    return (-ENOSYS);
}

/**
 * @brief Check if UART RX buffer has a received char
 *
 * @details Check if UART RX buffer has at least one pending character
 * (i.e. uart_fifo_read() will succeed and return non-zero). This function
 * must be called in a UART interrupt handler, or its result is undefined.
 * Before calling this function in the interrupt handler, uart_irq_update()
 * must be called once per the handler invocation. It's unspecified whether
 * condition as returned by this function is level- or edge- triggered (i.e.
 * if this function returns true when RX FIFO is non-empty, or when a new
 * char was received since last call to it). See description of
 * uart_fifo_read() for implication of this.
 *
 * @param dev UART device instance.
 *
 * @retval 1 If a received char is ready.
 * @retval 0 If a received char is not ready.
 * @retval -ENOSYS If this function is not implemented.
 * @retval -ENOTSUP If API is not enabled.
 */
static inline int uart_irq_rx_ready(void const* dev) {
    return (-ENOSYS);
}

/**
 * @brief Enable error interrupt.
 *
 * @param dev UART device instance.
 */
static inline void uart_irq_err_enable(void const* dev) {
    // pass
}

/**
 * @brief Disable error interrupt.
 *
 * @param dev UART device instance.
 */
static inline void uart_irq_err_disable(void const* dev) {
    // pass
}

/**
 * @brief Check if any IRQs is pending.
 *
 * @param dev UART device instance.
 *
 * @retval 1 If an IRQ is pending.
 * @retval 0 If an IRQ is not pending.
 * @retval -ENOSYS If this function is not implemented.
 * @retval -ENOTSUP If API is not enabled.
 */
static inline int uart_irq_is_pending(void const* dev) {
    return (-ENOSYS);
}

/**
 * @brief Start processing interrupts in ISR.
 *
 * This function should be called the first thing in the ISR. Calling
 * uart_irq_rx_ready(), uart_irq_tx_ready(), uart_irq_tx_complete()
 * allowed only after this.
 *
 * The purpose of this function is:
 *
 * * For devices with auto-acknowledge of interrupt status on register
 *   read to cache the value of this register (rx_ready, etc. then use
 *   this case).
 * * For devices with explicit acknowledgment of interrupts, to ack
 *   any pending interrupts and likewise to cache the original value.
 * * For devices with implicit acknowledgment, this function will be
 *   empty. But the ISR must perform the actions needs to ack the
 *   interrupts (usually, call uart_fifo_read() on rx_ready, and
 *   uart_fifo_fill() on tx_ready).
 *
 * @param dev UART device instance.
 *
 * @retval 1 On success.
 * @retval -ENOSYS If this function is not implemented.
 * @retval -ENOTSUP If API is not enabled.
 */
static inline int uart_irq_update(void const* dev) {
    return (-ENOSYS);
}

/**
 * @brief Set the IRQ callback function pointer.
 *
 * This sets up the callback for IRQ. When an IRQ is triggered,
 * the specified function will be called with specified user data.
 * See description of uart_irq_update() for the requirements on ISR.
 *
 * @param dev UART device instance.
 * @param cb Pointer to the callback function.
 * @param user_data Data to pass to callback function.
 *
 * @retval 0 On success.
 * @retval -ENOSYS If this function is not implemented.
 * @retval -ENOTSUP If API is not enabled.
 */
static inline int uart_irq_callback_user_data_set(void const* dev,
                                                  uart_irq_callback_user_data_t cb,
                                                  void* user_data) {
    return (-ENOSYS);
}

/**
 * @brief Set the IRQ callback function pointer (legacy).
 *
 * This sets up the callback for IRQ. When an IRQ is triggered,
 * the specified function will be called with the device pointer.
 *
 * @param dev UART device instance.
 * @param cb Pointer to the callback function.
 *
 * @retval 0 On success.
 * @retval -ENOSYS If this function is not implemented.
 * @retval -ENOTSUP If API is not enabled.
 */
static inline int uart_irq_callback_set(void const* dev,
                                        uart_irq_callback_user_data_t cb) {
    return uart_irq_callback_user_data_set(dev, cb, NULL);
}

/**
 * @}
 */

/**
 * @addtogroup uart_async
 * @{
 */

/**
 * @brief Set event handler function.
 *
 * Since it is mandatory to set callback to use other asynchronous functions,
 * it can be used to detect if the device supports asynchronous API. Remaining
 * API does not have that detection.
 *
 * @param dev       UART device instance.
 * @param callback  Event handler.
 * @param user_data Data to pass to event handler function.
 *
 * @retval 0 If successful.
 * @retval -ENOSYS If not supported by the device.
 * @retval -ENOTSUP If API not enabled.
 */
static inline int uart_callback_set(void const* dev, uart_callback_t callback, void* user_data) {
    return (-ENOSYS);
}

/**
 * @brief Send given number of bytes from buffer through UART.
 *
 * Function returns immediately and event handler,
 * set using @ref uart_callback_set, is called after transfer is finished.
 *
 * @param dev     UART device instance.
 * @param buf     Pointer to transmit buffer.
 * @param len     Length of transmit buffer.
 * @param tmout_us Timeout in microseconds. Valid only if flow control is
 *          enabled. @ref SYS_FOREVER_US disables timeout.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP If API is not enabled.
 * @retval -EBUSY If There is already an ongoing transfer.
 * @retval -errno Other negative errno value in case of failure.
 */
static inline int uart_tx(void const* dev, const uint8_t* buf, size_t len, int32_t tmout_us) {
    return (-ENOTSUP);
}

/**
 * @brief Send given number of datum from buffer through UART.
 *
 * Function returns immediately and event handler,
 * set using @ref uart_callback_set, is called after transfer is finished.
 *
 * @param dev     UART device instance.
 * @param buf     Pointer to wide data transmit buffer.
 * @param len     Length of wide data transmit buffer.
 * @param tmout_ms Timeout in milliseconds. Valid only if flow control is
 *          enabled. @ref SYS_FOREVER_MS disables timeout.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP If API is not enabled.
 * @retval -EBUSY If there is already an ongoing transfer.
 * @retval -errno Other negative errno value in case of failure.
 */
static inline int uart_tx_u16(void const* dev, const uint16_t* buf, size_t len, int32_t tmout_ms) {
    return (-ENOTSUP); 
}

/**
 * @brief Abort current TX transmission.
 *
 * #UART_TX_DONE event will be generated with amount of data sent.
 *
 * @param dev UART device instance.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP If API is not enabled.
 * @retval -EFAULT There is no active transmission.
 * @retval -errno Other negative errno value in case of failure.
 */
static inline int uart_tx_abort(void const* dev) {
    return (-ENOTSUP);
}

/**
 * @brief Start receiving data through UART.
 *
 * Function sets given buffer as first buffer for receiving and returns
 * immediately. After that event handler, set using @ref uart_callback_set,
 * is called with #UART_RX_RDY or #UART_RX_BUF_REQUEST events.
 *
 * @param dev     UART device instance.
 * @param buf     Pointer to receive buffer.
 * @param len     Buffer length.
 * @param tmout_us Inactivity period after receiving at least a byte which
 *        triggers  #UART_RX_RDY event. Given in microseconds.
 * @ref SYS_FOREVER_US disables timeout. See @ref uart_event_type
 *      for details.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP If API is not enabled.
 * @retval -EBUSY RX already in progress.
 * @retval -errno Other negative errno value in case of failure.
 *
 */
static inline int uart_rx_enable(void const* dev, uint8_t* buf, size_t len, int32_t tmout_us) {
    return (-ENOTSUP);
}

/**
 * @brief Start receiving wide data through UART.
 *
 * Function sets given buffer as first buffer for receiving and returns
 * immediately. After that event handler, set using @ref uart_callback_set,
 * is called with #UART_RX_RDY or #UART_RX_BUF_REQUEST events.
 *
 * @param dev     UART device instance.
 * @param buf     Pointer to wide data receive buffer.
 * @param len     Buffer length.
 * @param timeout_ms Inactivity period after receiving at least a byte which
 *        triggers  #UART_RX_RDY event. Given in milliseconds.
 *        @ref SYS_FOREVER_MS disables timeout. See
 *        @ref uart_event_type for details.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP If API is not enabled.
 * @retval -EBUSY RX already in progress.
 * @retval -errno Other negative errno value in case of failure.
 *
 */
static inline int uart_rx_enable_u16(void const* dev, uint16_t* buf, size_t len, int32_t tmout_ms) {
    return (-ENOTSUP);
}

/**
 * @brief Provide receive buffer in response to #UART_RX_BUF_REQUEST event.
 *
 * Provide pointer to RX buffer, which will be used when current buffer is
 * filled.
 *
 * @note Providing buffer that is already in usage by driver leads to
 *       undefined behavior. Buffer can be reused when it has been released
 *       by driver.
 *
 * @param dev UART device instance.
 * @param buf Pointer to receive buffer.
 * @param len Buffer length.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP If API is not enabled.
 * @retval -EBUSY Next buffer already set.
 * @retval -EACCES Receiver is already disabled (function called too late?).
 * @retval -errno Other negative errno value in case of failure.
 */
static inline int uart_rx_buf_rsp(void const* dev, uint8_t* buf, size_t len) {
    return (-ENOTSUP);
}

/**
 * @brief Provide wide data receive buffer in response to #UART_RX_BUF_REQUEST
 * event.
 *
 * Provide pointer to RX buffer, which will be used when current buffer is
 * filled.
 *
 * @note Providing buffer that is already in usage by driver leads to
 *       undefined behavior. Buffer can be reused when it has been released
 *       by driver.
 *
 * @param dev UART device instance.
 * @param buf Pointer to wide data receive buffer.
 * @param len Buffer length.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP If API is not enabled
 * @retval -EBUSY Next buffer already set.
 * @retval -EACCES Receiver is already disabled (function called too late?).
 * @retval -errno Other negative errno value in case of failure.
 */
static inline int uart_rx_buf_rsp_u16(void const* dev, uint16_t* buf, size_t len) {
    return (-ENOTSUP);
}

/**
 * @brief Disable RX
 *
 * #UART_RX_BUF_RELEASED event will be generated for every buffer scheduled,
 * after that #UART_RX_DISABLED event will be generated. Additionally, if there
 * is any pending received data, the #UART_RX_RDY event for that data will be
 * generated before the #UART_RX_BUF_RELEASED events.
 *
 * @param dev UART device instance.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP If API is not enabled.
 * @retval -EFAULT There is no active reception.
 * @retval -errno Other negative errno value in case of failure.
 */
static inline int uart_rx_disable(void const* dev) {
    return (-ENOTSUP);
}

/**
 * @}
 */

/**
 * @brief Manipulate line control for UART.
 *
 * @param dev UART device instance.
 * @param ctrl The line control to manipulate (see enum uart_line_ctrl).
 * @param val Value to set to the line control.
 *
 * @retval 0 If successful.
 * @retval -ENOSYS If this function is not implemented.
 * @retval -ENOTSUP If API is not enabled.
 * @retval -errno Other negative errno value in case of failure.
 */
static inline int uart_line_ctrl_set(void const* dev, uint32_t ctrl, uint32_t val) {
    return (-ENOTSUP);
}

/**
 * @brief Retrieve line control for UART.
 *
 * @param dev UART device instance.
 * @param ctrl The line control to retrieve (see enum uart_line_ctrl).
 * @param val Pointer to variable where to store the line control value.
 *
 * @retval 0 If successful.
 * @retval -ENOSYS If this function is not implemented.
 * @retval -ENOTSUP If API is not enabled.
 * @retval -errno Other negative errno value in case of failure.
 */
static inline int uart_line_ctrl_get(void const* dev, uint32_t ctrl, uint32_t* val) {
    return (-ENOTSUP);
}

/**
 * @brief Send extra command to driver.
 *
 * Implementation and accepted commands are driver specific.
 * Refer to the drivers for more information.
 *
 * @param dev UART device instance.
 * @param cmd Command to driver.
 * @param p Parameter to the command.
 *
 * @retval 0 If successful.
 * @retval -ENOSYS If this function is not implemented.
 * @retval -ENOTSUP If API is not enabled.
 * @retval -errno Other negative errno value in case of failure.
 */
static inline int uart_drv_cmd(void const* dev, uint32_t cmd, uint32_t p) {
    return (-ENOTSUP);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_UART_H_ */
