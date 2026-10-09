/**
 * @file stm32g4_uart.h
 * @brief Hardware-abstraction API for the STM32G4 USART/UART peripherals.
 *
 * This layer owns UART handle lifetime and requested communication settings.
 * It deliberately does not access RCC, GPIO, NVIC, or USART registers. Those
 * operations belong to the low-level implementation added in a later phase.
 */
#ifndef STM32G4_UART_H
#define STM32G4_UART_H

#include <stddef.h>
#include <stdint.h>

#define UART_WAIT_FOREVER          UINT32_MAX
#define UART_POLL_TIMEOUT_US       (2000000U)

/** USART/UART peripherals covered by this driver (not LPUART1). */
typedef enum
{
    UART_INSTANCE_USART1 = 0U,
    UART_INSTANCE_USART2,
    UART_INSTANCE_USART3,
    UART_INSTANCE_UART4,
    UART_INSTANCE_UART5,
    UART_INSTANCE_LPUART1,
    UART_INSTANCE_COUNT
} uart_instance_t;

/* Kept as an alias while existing code transitions to UART_INSTANCE_COUNT. */
#define UART_INST_COUNT UART_INSTANCE_COUNT

typedef enum
{
    UART_CLOSED = 0U,
    UART_OPEN
} uart_open_state_t;

typedef enum
{
    UART_OK = 0U,
    UART_TX_BUSY,
    UART_RX_BUSY,
    UART_ERROR,
    UART_INVALID_PARAM,
    UART_NOT_OPEN,
    UART_TIMEOUT,
    UART_NOT_SUPPORTED,
    UART_INVALID_CONTEXT
} uart_status;

typedef enum
{
    UART_WORD_LENGTH_7 = 0U,
    UART_WORD_LENGTH_8,
    UART_WORD_LENGTH_9
} uart_word_length_t;

typedef enum
{
    UART_PARITY_NONE = 0U,
    UART_PARITY_EVEN,
    UART_PARITY_ODD
} uart_parity_t;

typedef enum
{
    UART_STOP_BITS_1 = 0U,
    UART_STOP_BITS_2
} uart_stop_bits_t;

typedef enum
{
    UART_OVERSAMPLING_16 = 0U,
    UART_OVERSAMPLING_8
} uart_oversampling_t;

typedef enum
{
    UART_MODE_TX = 0U,
    UART_MODE_RX,
    UART_MODE_TX_RX
} uart_mode_t;

typedef enum
{
    UART_FIFO_DISABLED = 0U,
    UART_FIFO_ENABLED
} uart_fifo_mode_t;

typedef enum
{
    UART_FLOW_CONTROL_NONE = 0U,
    UART_FLOW_CONTROL_RTS,
    UART_FLOW_CONTROL_CTS,
    UART_FLOW_CONTROL_RTS_CTS
} uart_flow_control_t;

struct uart_descriptor;
typedef struct uart_descriptor *uart_handle_t;

/** Callback invoked when an async transfer completes (or fails). */
typedef void (*uart_callback_t)(uart_handle_t uart, uart_status status, size_t transferred);

/** Communication settings requested for one UART instance. */
typedef struct
{
    uint32_t baudrate;
    uart_word_length_t word_length;
    uart_parity_t parity;
    uart_stop_bits_t stop_bits;
    uart_oversampling_t oversampling;
    uart_mode_t mode;
    uart_fifo_mode_t fifo_mode;
    uart_flow_control_t flow_control;
    uart_callback_t tx_cb;
    uart_callback_t rx_cb;
} uart_config_t;

/**
 * Opaque, driver-owned UART descriptor.
 *
 * Its layout is deliberately private to stm32g4_uart.c, so application code
 * can only pass a handle to UART APIs and cannot alter driver state.
 */

/**
 * Acquire the driver-owned handle for an instance.
 *
 * A valid, already-open instance returns its existing descriptor without
 * altering its configuration. NULL denotes an invalid instance.
 */
uart_handle_t uart_open(uart_instance_t instance);

/**
 * Validate and store communication settings for an open, idle descriptor.
 *
 * No hardware is configured by this HAL-only implementation.
 */
uart_status uart_config(uart_handle_t uart, const uart_config_t *config);

/**
 * Transmit synchronously by polling the UART data-ready and completion flags.
 *
 * This API is scheduler-independent and may be called before FreeRTOS starts.
 * The driver uses UART_POLL_TIMEOUT_US as its fixed timeout. written may be NULL.
 */
uart_status uart_write_polling(
    uart_handle_t uart,
    const uint8_t *buffer,
    size_t length,
    size_t *written);

/**
 * Receive synchronously by polling the UART receive-ready and error flags.
 *
 * This API is scheduler-independent and may be called before FreeRTOS starts.
 * The driver uses UART_POLL_TIMEOUT_US as its fixed timeout. read may be NULL.
 */
uart_status uart_read_polling(
    uart_handle_t uart,
    uint8_t *buffer,
    size_t length,
    size_t *read);

/**
 * Transmit using UART interrupts, blocking the calling FreeRTOS task until
 * completion or timeout.
 */
uart_status uart_write_sync(
    uart_handle_t uart,
    const uint8_t *buffer,
    size_t length,
    uint32_t timeout_ticks,
    size_t *written);

/**
 * Transmit asynchronously using UART interrupts. Returns immediately.
 * The tx_cb callback in uart_config_t will be invoked upon completion.
 */
uart_status uart_write_async(
    uart_handle_t uart,
    const uint8_t *buffer,
    size_t length);

/**
 * Receive using UART interrupts, blocking the calling FreeRTOS task until
 * completion, error, or timeout.
 */
uart_status uart_read_sync(
    uart_handle_t uart,
    uint8_t *buffer,
    size_t length,
    uint32_t timeout_ticks,
    size_t *read);

/**
 * Receive asynchronously using UART interrupts. Returns immediately.
 * The rx_cb callback in uart_config_t will be invoked upon completion.
 */
uart_status uart_read_async(
    uart_handle_t uart,
    uint8_t *buffer,
    size_t length);

/**
 * Generic Interrupt Service Routine (ISR) handler for UART.
 * 
 * This function should be called from the actual hardware interrupt handlers
 * (e.g., USART1_IRQHandler) once the NVIC is configured. It processes
 * TX/RX logic, manages state, and triggers FreeRTOS task wakeups or user callbacks.
 */
void uart_irq_handler(uart_handle_t uart);

/** Release an open descriptor and clear all driver-owned state. */
uart_status uart_close(uart_handle_t uart);

#endif /* STM32G4_UART_H */
