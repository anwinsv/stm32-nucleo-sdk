/**
 * @file stm32g4_uart.c
 * @brief Hardware-abstraction implementation for STM32G4 UART instances.
 */
#include "stm32g4_uart.h"
#include "stm32g4_uart_ll.h"
#include "FreeRTOS.h"
#include "task.h"

typedef enum
{
    UART_TRANSFER_IDLE = 0U,
    UART_TRANSFER_ACTIVE,
    UART_TRANSFER_COMPLETE,
    UART_TRANSFER_FAILED,
    UART_TRANSFER_TIMED_OUT
} uart_transfer_state_t;

typedef struct
{
    const uint8_t *buffer;
    size_t length;
    size_t completed;
    uart_transfer_state_t state;
    uart_status result;
} uart_tx_transfer_t;

typedef struct
{
    uint8_t *buffer;
    size_t length;
    size_t completed;
    uart_transfer_state_t state;
    uart_status result;
} uart_rx_transfer_t;

struct uart_descriptor
{
    uart_instance_t instance;
    uart_open_state_t is_open;
    uart_status status;
    uart_config_t config;

    uart_tx_transfer_t tx;
    uart_rx_transfer_t rx;
    
    TaskHandle_t tx_task;
    TaskHandle_t rx_task;
};

static struct uart_descriptor uart_instances[UART_INSTANCE_COUNT];

static const uart_config_t uart_default_config =
{
    .baudrate = 115200U,
    .word_length = UART_WORD_LENGTH_8,
    .parity = UART_PARITY_NONE,
    .stop_bits = UART_STOP_BITS_1,
    .oversampling = UART_OVERSAMPLING_16,
    .mode = UART_MODE_TX_RX,
    .fifo_mode = UART_FIFO_DISABLED,
    .flow_control = UART_FLOW_CONTROL_NONE
};

static uint8_t uart_instance_is_valid(uart_instance_t instance)
{
    return ((uint32_t)instance < (uint32_t)UART_INSTANCE_COUNT);
}

static uint8_t uart_handle_is_valid(uart_handle_t uart)
{
    uint32_t index;

    for (index = 0U; index < (uint32_t)UART_INSTANCE_COUNT; ++index)
    {
        if (uart == &uart_instances[index])
        {
            return 1U;
        }
    }

    return 0U;
}

static uint8_t uart_config_is_valid(const uart_config_t *config)
{
    if ((config == NULL) || (config->baudrate == 0U))
    {
        return 0U;
    }

    if (((uint32_t)config->word_length > (uint32_t)UART_WORD_LENGTH_9) ||
        ((uint32_t)config->parity > (uint32_t)UART_PARITY_ODD) ||
        ((uint32_t)config->stop_bits > (uint32_t)UART_STOP_BITS_2) ||
        ((uint32_t)config->oversampling > (uint32_t)UART_OVERSAMPLING_8) ||
        ((uint32_t)config->mode > (uint32_t)UART_MODE_TX_RX) ||
        ((uint32_t)config->fifo_mode > (uint32_t)UART_FIFO_ENABLED) ||
        ((uint32_t)config->flow_control > (uint32_t)UART_FLOW_CONTROL_RTS_CTS))
    {
        return 0U;
    }

    return 1U;
}

static uint8_t uart_mode_has_tx(uart_mode_t mode)
{
    return ((mode == UART_MODE_TX) || (mode == UART_MODE_TX_RX));
}

static uint8_t uart_mode_has_rx(uart_mode_t mode)
{
    return ((mode == UART_MODE_RX) || (mode == UART_MODE_TX_RX));
}

static uint8_t uart_tx_is_active(const struct uart_descriptor *uart)
{
    return (uart->tx.state == UART_TRANSFER_ACTIVE);
}

static uint8_t uart_rx_is_active(const struct uart_descriptor *uart)
{
    return (uart->rx.state == UART_TRANSFER_ACTIVE);
}

static uart_status uart_validate_transfer(
    uart_handle_t uart,
    const void *buffer,
    size_t length,
    uint8_t is_tx)
{
    if (uart_handle_is_valid(uart) == 0U)
    {
        return UART_INVALID_PARAM;
    }

    if (uart->is_open != UART_OPEN)
    {
        return UART_NOT_OPEN;
    }

    if ((buffer == NULL) || (length == 0U))
    {
        return UART_INVALID_PARAM;
    }

    if ((is_tx != 0U) && (uart_mode_has_tx(uart->config.mode) == 0U))
    {
        return UART_ERROR;
    }

    if ((is_tx == 0U) && (uart_mode_has_rx(uart->config.mode) == 0U))
    {
        return UART_ERROR;
    }

    if ((is_tx != 0U) && (uart_tx_is_active(uart) != 0U))
    {
        return UART_TX_BUSY;
    }

    if ((is_tx == 0U) && (uart_rx_is_active(uart) != 0U))
    {
        return UART_RX_BUSY;
    }

    return UART_OK;
}

static void uart_reset_descriptor(uart_handle_t uart, uart_instance_t instance)
{
    *uart = (struct uart_descriptor)
    {
        .instance = instance,
        .is_open = UART_CLOSED,
        .status = UART_OK,
        .config = uart_default_config,
        .tx =
        {
            .buffer = NULL,
            .length = 0U,
            .completed = 0U,
            .state = UART_TRANSFER_IDLE,
            .result = UART_OK
        },
        .rx =
        {
            .buffer = NULL,
            .length = 0U,
            .completed = 0U,
            .state = UART_TRANSFER_IDLE,
            .result = UART_OK
        }
    };
}

uart_handle_t uart_open(uart_instance_t instance)
{
    uart_handle_t uart;

    if (uart_instance_is_valid(instance) == 0U)
    {
        return NULL;
    }

    uart = &uart_instances[(uint32_t)instance];

    if (uart->is_open == UART_CLOSED)
    {
        uart_reset_descriptor(uart, instance);
        uart->is_open = UART_OPEN;

        uart_ll_enable_clocks(instance);
        uart_ll_init_gpio(instance);
    }

    return uart;
}

uart_status uart_config(uart_handle_t uart, const uart_config_t *config)
{
    if ((uart_handle_is_valid(uart) == 0U) || (uart_config_is_valid(config) == 0U))
    {
        return UART_INVALID_PARAM;
    }

    if (uart->is_open != UART_OPEN)
    {
        return UART_NOT_OPEN;
    }

    if (uart_tx_is_active(uart) != 0U)
    {
        return UART_TX_BUSY;
    }

    if (uart_rx_is_active(uart) != 0U)
    {
        return UART_RX_BUSY;
    }

    uart->config = *config;
    uart->status = UART_OK;

    return uart_ll_configure(uart->instance, config);
}

uart_status uart_write_polling(
    uart_handle_t uart,
    const uint8_t *buffer,
    size_t length,
    size_t *written)
{
    uart_status status;
    size_t i;
    uint32_t timeout;

    if (written != NULL)
    {
        *written = 0U;
    }

    status = uart_validate_transfer(uart, buffer, length, 1U);
    if (status != UART_OK)
    {
        return status;
    }

    uart->tx.state = UART_TRANSFER_ACTIVE;
    uart->tx.buffer = buffer;
    uart->tx.length = length;
    uart->tx.completed = 0U;

    for (i = 0U; i < length; ++i)
    {
        /* Crude software loop timeout (factor of 10 to account for loop instructions) */
        timeout = UART_POLL_TIMEOUT_US * 10U; 
        while (uart_ll_is_tx_empty(uart->instance) == 0U)
        {
            if (timeout == 0U)
            {
                uart->tx.state = UART_TRANSFER_TIMED_OUT;
                return UART_TIMEOUT;
            }
            timeout--;
        }

        uart_ll_write_byte(uart->instance, buffer[i]);
        uart->tx.completed++;
        
        if (written != NULL)
        {
            *written = uart->tx.completed;
        }
    }

    /* Wait for transmission complete (TC) before leaving */
    timeout = UART_POLL_TIMEOUT_US * 10U;
    while (uart_ll_is_tx_complete(uart->instance) == 0U)
    {
        if (timeout == 0U)
        {
            uart->tx.state = UART_TRANSFER_TIMED_OUT;
            return UART_TIMEOUT;
        }
        timeout--;
    }

    uart->tx.state = UART_TRANSFER_IDLE;
    return UART_OK;
}

uart_status uart_read_polling(
    uart_handle_t uart,
    uint8_t *buffer,
    size_t length,
    size_t *read)
{
    uart_status status;
    size_t i;
    uint32_t timeout;

    if (read != NULL)
    {
        *read = 0U;
    }

    status = uart_validate_transfer(uart, buffer, length, 0U);
    if (status != UART_OK)
    {
        return status;
    }

    uart->rx.state = UART_TRANSFER_ACTIVE;
    uart->rx.buffer = buffer;
    uart->rx.length = length;
    uart->rx.completed = 0U;

    for (i = 0U; i < length; ++i)
    {
        /* Crude software loop timeout (factor of 10 to account for loop instructions) */
        timeout = UART_POLL_TIMEOUT_US * 10U; 
        while (uart_ll_is_rx_ready(uart->instance) == 0U)
        {
            if (timeout == 0U)
            {
                uart->rx.state = UART_TRANSFER_TIMED_OUT;
                return UART_TIMEOUT;
            }
            timeout--;
        }

        buffer[i] = uart_ll_read_byte(uart->instance);
        uart->rx.completed++;
        
        if (read != NULL)
        {
            *read = uart->rx.completed;
        }
    }

    uart->rx.state = UART_TRANSFER_IDLE;
    return UART_OK;
}

static uart_status uart_write_internal(
    uart_handle_t uart,
    const uint8_t *buffer,
    size_t length,
    uint8_t is_sync)
{
    uart_status status = uart_validate_transfer(uart, buffer, length, 1U);
    if (status != UART_OK)
    {
        return status;
    }

    uart->tx.state = UART_TRANSFER_ACTIVE;
    uart->tx.buffer = buffer;
    uart->tx.length = length;
    uart->tx.completed = 0U;
    
    if (is_sync)
    {
        /* Store the calling task's handle so the ISR knows who to notify */
        uart->tx_task = xTaskGetCurrentTaskHandle();
        /* Clear any pending notifications before enabling interrupts */
        ulTaskNotifyTake(pdTRUE, 0);
    }
    else
    {
        uart->tx_task = NULL;
    }

    /* Enable TXE interrupt to jump into the ISR and start the transfer */
    uart_ll_enable_tx_interrupt(uart->instance);
    return UART_OK;
}

uart_status uart_write_sync(
    uart_handle_t uart,
    const uint8_t *buffer,
    size_t length,
    uint32_t timeout_ticks,
    size_t *written)
{
    uart_status status;
    uint32_t notified_val;

    if (written != NULL)
    {
        *written = 0U;
    }

    status = uart_write_internal(uart, buffer, length, 1U);
    if (status != UART_OK)
    {
        return status;
    }

    /* Block the task until the ISR notifies completion or we time out */
    notified_val = ulTaskNotifyTake(pdTRUE, timeout_ticks);

    if (notified_val == 0U)
    {
        /* Timeout occurred */
        uart_ll_disable_tx_interrupt(uart->instance);
        uart_ll_disable_tc_interrupt(uart->instance);
        uart->tx.state = UART_TRANSFER_TIMED_OUT;
        
        if (written != NULL)
        {
            *written = uart->tx.completed;
        }
        return UART_TIMEOUT;
    }

    if (written != NULL)
    {
        *written = uart->tx.completed;
    }
    
    return UART_OK;
}

uart_status uart_write_async(
    uart_handle_t uart,
    const uint8_t *buffer,
    size_t length)
{
    return uart_write_internal(uart, buffer, length, 0U);
}

static uart_status uart_read_internal(
    uart_handle_t uart,
    uint8_t *buffer,
    size_t length,
    uint8_t is_sync)
{
    uart_status status = uart_validate_transfer(uart, buffer, length, 0U);
    if (status != UART_OK)
    {
        return status;
    }

    uart->rx.state = UART_TRANSFER_ACTIVE;
    uart->rx.buffer = buffer;
    uart->rx.length = length;
    uart->rx.completed = 0U;
    
    if (is_sync)
    {
        uart->rx_task = xTaskGetCurrentTaskHandle();
        ulTaskNotifyTake(pdTRUE, 0);
    }
    else
    {
        uart->rx_task = NULL;
    }

    uart_ll_enable_rx_interrupt(uart->instance);
    return UART_OK;
}

uart_status uart_read_sync(
    uart_handle_t uart,
    uint8_t *buffer,
    size_t length,
    uint32_t timeout_ticks,
    size_t *read)
{
    uart_status status;
    uint32_t notified_val;

    if (read != NULL)
    {
        *read = 0U;
    }

    status = uart_read_internal(uart, buffer, length, 1U);
    if (status != UART_OK)
    {
        return status;
    }

    notified_val = ulTaskNotifyTake(pdTRUE, timeout_ticks);

    if (notified_val == 0U)
    {
        uart_ll_disable_rx_interrupt(uart->instance);
        uart->rx.state = UART_TRANSFER_TIMED_OUT;
        
        if (read != NULL)
        {
            *read = uart->rx.completed;
        }
        return UART_TIMEOUT;
    }

    if (read != NULL)
    {
        *read = uart->rx.completed;
    }
    
    return UART_OK;
}

uart_status uart_read_async(
    uart_handle_t uart,
    uint8_t *buffer,
    size_t length)
{
    return uart_read_internal(uart, buffer, length, 0U);
}

void uart_irq_handler(uart_handle_t uart)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if ((uart == NULL) || (uart->is_open != UART_OPEN))
    {
        return;
    }

    /* -------------------------------------------------------------
     * TRANSMIT LOGIC
     * ------------------------------------------------------------- */
    if ((uart_ll_is_tx_interrupt_enabled(uart->instance) != 0U) &&
        (uart_ll_is_tx_empty(uart->instance) != 0U))
    {
        if (uart->tx.completed < uart->tx.length)
        {
            uart_ll_write_byte(uart->instance, uart->tx.buffer[uart->tx.completed]);
            uart->tx.completed++;
        }
        else
        {
            /* All bytes written to TDR. Disable TXE interrupt, wait for TC */
            uart_ll_disable_tx_interrupt(uart->instance);
            uart_ll_enable_tc_interrupt(uart->instance);
        }
    }

    if ((uart_ll_is_tc_interrupt_enabled(uart->instance) != 0U) &&
        (uart_ll_is_tx_complete(uart->instance) != 0U))
    {
        /* Transmission fully complete */
        uart_ll_disable_tc_interrupt(uart->instance);
        uart->tx.state = UART_TRANSFER_IDLE;

        if (uart->tx_task != NULL)
        {
            /* Sync transfer: Wake the waiting task */
            vTaskNotifyGiveFromISR(uart->tx_task, &xHigherPriorityTaskWoken);
        }
        else if (uart->config.tx_cb != NULL)
        {
            /* Async transfer: Call the user callback */
            uart->config.tx_cb(uart, UART_OK, uart->tx.completed);
        }
    }

    /* -------------------------------------------------------------
     * RECEIVE LOGIC
     * ------------------------------------------------------------- */
    if ((uart_ll_is_rx_interrupt_enabled(uart->instance) != 0U) &&
        (uart_ll_is_rx_ready(uart->instance) != 0U))
    {
        if (uart->rx.completed < uart->rx.length)
        {
            uart->rx.buffer[uart->rx.completed] = uart_ll_read_byte(uart->instance);
            uart->rx.completed++;
        }

        if (uart->rx.completed >= uart->rx.length)
        {
            /* Reception fully complete */
            uart_ll_disable_rx_interrupt(uart->instance);
            uart->rx.state = UART_TRANSFER_IDLE;

            if (uart->rx_task != NULL)
            {
                /* Sync transfer: Wake the waiting task */
                vTaskNotifyGiveFromISR(uart->rx_task, &xHigherPriorityTaskWoken);
            }
            else if (uart->config.rx_cb != NULL)
            {
                /* Async transfer: Call the user callback */
                uart->config.rx_cb(uart, UART_OK, uart->rx.completed);
            }
        }
    }

    /* Yield if a higher priority task was woken by the notifications */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

uart_status uart_close(uart_handle_t uart)
{
    uart_instance_t instance;

    if (uart_handle_is_valid(uart) == 0U)
    {
        return UART_INVALID_PARAM;
    }

    if (uart->is_open != UART_OPEN)
    {
        return UART_NOT_OPEN;
    }

    if (uart_tx_is_active(uart) != 0U)
    {
        return UART_TX_BUSY;
    }

    if (uart_rx_is_active(uart) != 0U)
    {
        return UART_RX_BUSY;
    }

    instance = uart->instance;
    uart_ll_disable(instance);

    uart_reset_descriptor(uart, instance);

    return UART_OK;
}
