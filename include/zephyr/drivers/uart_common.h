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

#ifndef ZEPHYR_INCLUDE_DRIVERS_UART_COMMON_H_
#define ZEPHYR_INCLUDE_DRIVERS_UART_COMMON_H_

/**
 * @brief UART Interface
 * @defgroup uart_interface UART Interface
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>
#include <stddef.h>
#include <zephyr/sys/util_macro.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Line control signals. */
enum uart_line_ctrl {
    UART_LINE_CTRL_BAUD_RATE = BIT(0),
    UART_LINE_CTRL_RTS = BIT(1),
    UART_LINE_CTRL_DTR = BIT(2),
    UART_LINE_CTRL_DCD = BIT(3),
    UART_LINE_CTRL_DSR = BIT(4)
};

/**
 * @brief Reception stop reasons.
 *
 * Values that correspond to events or errors responsible for stopping
 * receiving.
 */
enum uart_rx_stop_reason {
    /** @brief Overrun error */
    UART_ERROR_OVERRUN = (1 << 0),
    /** @brief Parity error */
    UART_ERROR_PARITY  = (1 << 1),
    /** @brief Framing error */
    UART_ERROR_FRAMING = (1 << 2),
    /**
     * @brief Break interrupt
     *
     * A break interrupt was received. This happens when the serial input
     * is held at a logic '0' state for longer than the sum of
     * start time + data bits + parity + stop bits.
     */
    UART_BREAK = (1 << 3),
    /**
     * @brief Collision error
     *
     * This error is raised when transmitted data does not match
     * received data. Typically this is useful in scenarios where
     * the TX and RX lines maybe connected together such as
     * RS-485 half-duplex. This error is only valid on UARTs that
     * support collision checking.
     */
    UART_ERROR_COLLISION = (1 << 4),

    /** @brief Noise error */
    UART_ERROR_NOISE = (1 << 5),
};

/** @brief Parity modes */
enum uart_config_parity {
    UART_CFG_PARITY_NONE,
    UART_CFG_PARITY_ODD,
    UART_CFG_PARITY_EVEN,
    UART_CFG_PARITY_MARK,
    UART_CFG_PARITY_SPACE
};

/** @brief Number of stop bits. */
enum uart_config_stop_bits {
    UART_CFG_STOP_BITS_0_5,
    UART_CFG_STOP_BITS_1,
    UART_CFG_STOP_BITS_1_5,
    UART_CFG_STOP_BITS_2
};

/** @brief Number of data bits. */
enum uart_config_data_bits {
    UART_CFG_DATA_BITS_5,
    UART_CFG_DATA_BITS_6,
    UART_CFG_DATA_BITS_7,
    UART_CFG_DATA_BITS_8,
    UART_CFG_DATA_BITS_9
};

/**
 * @brief Hardware flow control options.
 *
 * With flow control set to none, any operations related to flow control
 * signals can be managed by user with uart_line_ctrl functions.
 * In other cases, flow control is managed by hardware/driver.
 */
enum uart_config_flow_control {
    UART_CFG_FLOW_CTRL_NONE,
    UART_CFG_FLOW_CTRL_RTS_CTS,
    UART_CFG_FLOW_CTRL_DTR_DSR,
    UART_CFG_FLOW_CTRL_RS485,
};

/**
 * @brief UART controller configuration structure
 *
 * @param baudrate  Baudrate setting in bps
 * @param parity    Parity bit, use @ref uart_config_parity
 * @param stop_bits Stop bits, use @ref uart_config_stop_bits
 * @param data_bits Data bits, use @ref uart_config_data_bits
 * @param flow_ctrl Flow control setting, use @ref uart_config_flow_control
 */
struct uart_config {
    uint32_t baudrate;
    uint8_t parity;
    uint8_t stop_bits;
    uint8_t data_bits;
    uint8_t flow_ctrl;
};

/**
 * @defgroup uart_interrupt Interrupt-driven UART API
 * @{
 */

/**
 * @brief Define the application callback function signature for
 * uart_irq_callback_user_data_set() function.
 *
 * @param dev UART device instance.
 * @param user_data Arbitrary user data.
 */
typedef void (*uart_irq_callback_user_data_t)(const struct device* dev, void* user_data);

/**
 * @brief For configuring IRQ on each individual UART device.
 *
 * @param dev UART device instance.
 */
typedef void (*uart_irq_config_func_t)(const struct device* dev);

/**
 * @}
 *
 * @defgroup uart_async Async UART API
 * @{
 */

/**
 * @brief Types of events passed to callback in UART_ASYNC_API
 *
 * Receiving:
 * 1. To start receiving, uart_rx_enable has to be called with first buffer
 * 2. When receiving starts to current buffer,
 *    #UART_RX_BUF_REQUEST will be generated, in response to that user can
 *    either:
 *
 *    - Provide second buffer using uart_rx_buf_rsp, when first buffer is
 *      filled, receiving will automatically start to second buffer.
 *    - Ignore the event, this way when current buffer is filled
 *      #UART_RX_RDY event will be generated and receiving will be stopped.
 *
 * 3. If some data was received and timeout occurred #UART_RX_RDY event will be
 *    generated. It can happen multiples times for the same buffer. RX timeout
 *    is counted from last byte received i.e. if no data was received, there
 *    won't be any timeout event.
 * 4. After buffer is filled #UART_RX_RDY will be generated, immediately
 *    followed by #UART_RX_BUF_RELEASED indicating that current buffer
 *    is no longer used.
 * 5. If there was second buffer provided, it will become current buffer and
 *    we start again at point 2.
 *    If no second buffer was specified receiving is stopped and
 *    #UART_RX_DISABLED event is generated. After that whole process can be
 *    repeated.
 *
 * Any time during reception #UART_RX_STOPPED event can occur. if there is any
 * data received, #UART_RX_RDY event will be generated. It will be followed by
 * #UART_RX_BUF_RELEASED event for every buffer currently passed to driver and
 * finally by #UART_RX_DISABLED event.
 *
 * Receiving can be disabled using uart_rx_disable, after calling that
 * function, if there is any data received, #UART_RX_RDY event will be
 * generated. #UART_RX_BUF_RELEASED event will be generated for every buffer
 * currently passed to driver and finally #UART_RX_DISABLED event will occur.
 *
 * Transmitting:
 * 1. Transmitting starts by uart_tx function.
 * 2. If whole buffer was transmitted #UART_TX_DONE is generated. If timeout
 *    occurred #UART_TX_ABORTED will be generated.
 *
 * Transmitting can be aborted using @ref uart_tx_abort, after calling that
 * function #UART_TX_ABORTED event will be generated.
 *
 */
enum uart_event_type {
    /** @brief Whole TX buffer was transmitted. */
    UART_TX_DONE,
    /**
     * @brief Transmitting aborted due to timeout or uart_tx_abort call
     *
     * When flow control is enabled, there is a possibility that TX transfer
     * won't finish in the allotted time. Some data may have been
     * transferred, information about it can be found in event data.
     */
    UART_TX_ABORTED,
    /**
     * @brief Received data is ready for processing.
     *
     * This event is generated in the following cases:
     * - When RX timeout occurred, and data was stored in provided buffer.
     *   This can happen multiple times in the same buffer.
     * - When provided buffer is full.
     * - After uart_rx_disable().
     * - After stopping due to external event (#UART_RX_STOPPED).
     */
    UART_RX_RDY,
    /**
     * @brief Driver requests next buffer for continuous reception.
     *
     * This event is triggered when receiving has started for a new buffer,
     * i.e. it's time to provide a next buffer for a seamless switchover to
     * it. For continuous reliable receiving, user should provide another RX
     * buffer in response to this event, using uart_rx_buf_rsp function
     *
     * If uart_rx_buf_rsp is not called before current buffer
     * is filled up, receiving will stop.
     */
    UART_RX_BUF_REQUEST,
    /**
     * @brief Buffer is no longer used by UART driver.
     */
    UART_RX_BUF_RELEASED,
    /**
     * @brief RX has been disabled and can be reenabled.
     *
     * This event is generated whenever receiver has been stopped, disabled
     * or finished its operation and can be enabled again using
     * uart_rx_enable
     */
    UART_RX_DISABLED,
    /**
     * @brief RX has stopped due to external event.
     *
     * Reason is one of uart_rx_stop_reason.
     */
    UART_RX_STOPPED
};

/** @brief UART TX event data. */
struct uart_event_tx {
    /** @brief Pointer to current buffer. */
    const uint8_t *buf;
    /** @brief Number of bytes sent. */
    size_t len;
};

/**
 * @brief UART RX event data.
 *
 * The data represented by the event is stored in rx.buf[rx.offset] to
 * rx.buf[rx.offset+rx.len].  That is, the length is relative to the offset.
 */
struct uart_event_rx {
    /** @brief Pointer to current buffer. */
    uint8_t *buf;
    /** @brief Currently received data offset in bytes. */
    size_t offset;
    /** @brief Number of new bytes received. */
    size_t len;
};

/** @brief UART RX buffer released event data. */
struct uart_event_rx_buf {
    /* @brief Pointer to buffer that is no longer in use. */
    uint8_t *buf;
};

/** @brief UART RX stopped data. */
struct uart_event_rx_stop {
    /** @brief Reason why receiving stopped */
    enum uart_rx_stop_reason reason;
    /** @brief Last received data. */
    struct uart_event_rx data;
};

/** @brief Structure containing information about current event. */
struct uart_event {
    /** @brief Type of event */
    enum uart_event_type type;
    /** @brief Event data */
    union uart_event_data {
        /** @brief #UART_TX_DONE and #UART_TX_ABORTED events data. */
        struct uart_event_tx tx;
        /** @brief #UART_RX_RDY event data. */
        struct uart_event_rx rx;
        /** @brief #UART_RX_BUF_RELEASED event data. */
        struct uart_event_rx_buf rx_buf;
        /** @brief #UART_RX_STOPPED event data. */
        struct uart_event_rx_stop rx_stop;
    } data;
};

/**
 * @typedef uart_callback_t
 * @brief Define the application callback function signature for
 * uart_callback_set() function.
 *
 * @param dev UART device instance.
 * @param evt Pointer to uart_event instance.
 * @param user_data Pointer to data specified by user.
 */
typedef void (*uart_callback_t)(const struct device* dev,
                                struct uart_event* evt,
                                void* user_data);

struct zephyr_uart_cb_t {
    uart_callback_t async_cb;     
    char* const     async_rx_buf;
    size_t          async_rx_len;
    struct uart_config const* cfg;
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_UART_COMMON_H_ */
