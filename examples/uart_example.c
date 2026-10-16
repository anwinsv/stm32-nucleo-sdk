/**
 * @file    uart_example.c
 * @brief   UART and NVIC validation sandbox for STM32G4.
 */

#include "uart_example.h"
#include "stm32g4_uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stm32g4xx_hal.h"
#include <string.h>

#define UART_EX_BAUDRATE        (115200U)
#define UART_EX_TIMEOUT_MS      (1000U)
#define UART_EX_LINE_BUF_SIZE   (128U)

static void vUartExampleTask(void *pvParameters)
{
    (void)pvParameters;

    const uart_config_t config = {
        .baudrate     = UART_EX_BAUDRATE,
        .word_length  = UART_WORD_LENGTH_8,
        .parity       = UART_PARITY_NONE,
        .stop_bits    = UART_STOP_BITS_1,
        .oversampling = UART_OVERSAMPLING_16,
        .mode         = UART_MODE_TX_RX,
        .fifo_mode    = UART_FIFO_DISABLED,
        .flow_control = UART_FLOW_CONTROL_NONE,
        .tx_cb        = NULL,
        .rx_cb        = NULL
    };

    /* 1. Open LPUART1 (mapped to Nucleo ST-LINK VCP on PA2/PA3) */
    uart_handle_t uart = uart_open(UART_INSTANCE_LPUART1);
    if (uart == NULL)
    {
        vTaskDelete(NULL);
        return;
    }

    if (uart_config(uart, &config) != UART_OK)
    {
        vTaskDelete(NULL);
        return;
    }

    /* 2. Step 1: Polling transmission (Sanity check) */
    const char *msg_poll = "\r\n========================================\r\n"
                           " [Step 1] Polling TX: SUCCESS\r\n"
                           " Clock gating, BRR & GPIO AF configured.\r\n";
    (void)uart_write_polling(uart, (const uint8_t *)msg_poll, strlen(msg_poll), NULL);

    vTaskDelay(pdMS_TO_TICKS(100));

    /* 3. Step 2: Interrupt-driven synchronous transmission (NVIC + TXE/TC verification) */
    const char *msg_sync = " [Step 2] Interrupt Sync TX: SUCCESS\r\n"
                           " NVIC delivered TXE/TC IRQ; FreeRTOS task notified.\r\n"
                           "========================================\r\n"
                           " [Step 3] Interactive Echo Ready!\r\n"
                           " Type a message and press Enter:\r\n> ";
    (void)uart_write_sync(uart, (const uint8_t *)msg_sync, strlen(msg_sync), pdMS_TO_TICKS(UART_EX_TIMEOUT_MS), NULL);

    /* 4. Step 3: Interactive Line-based Echo Loop */
    char line_buf[UART_EX_LINE_BUF_SIZE];
    size_t line_len = 0;
    uint8_t rx_char = 0;
    size_t bytes_read = 0;

    for (;;)
    {
        /* Wait indefinitely for the next character via RX interrupt */
        uart_status status = uart_read_sync(
            uart,
            &rx_char,
            1U,
            portMAX_DELAY,
            &bytes_read
        );

        if ((status == UART_OK) && (bytes_read > 0U))
        {
            /* Visual feedback: toggle Nucleo Green LED (PA5) on each byte */
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);

            /* Ignore standalone Line Feed (\n) if PuTTY sends \r\n */
            if (rx_char == '\n')
            {
                continue;
            }

            /* Handle Enter (Carriage Return) -> Output explicit ECHO response */
            if (rx_char == '\r')
            {
                if (line_len > 0)
                {
                    line_buf[line_len] = '\0';

                    const char *prefix = "\r\n[ECHO]: \"";
                    const char *suffix = "\"\r\n> ";

                    (void)uart_write_sync(uart, (const uint8_t *)prefix, strlen(prefix), pdMS_TO_TICKS(UART_EX_TIMEOUT_MS), NULL);
                    (void)uart_write_sync(uart, (const uint8_t *)line_buf, line_len, pdMS_TO_TICKS(UART_EX_TIMEOUT_MS), NULL);
                    (void)uart_write_sync(uart, (const uint8_t *)suffix, strlen(suffix), pdMS_TO_TICKS(UART_EX_TIMEOUT_MS), NULL);

                    line_len = 0;
                }
                else
                {
                    const char *prompt = "\r\n> ";
                    (void)uart_write_sync(uart, (const uint8_t *)prompt, strlen(prompt), pdMS_TO_TICKS(UART_EX_TIMEOUT_MS), NULL);
                }
            }
            /* Handle Backspace / Delete */
            else if (rx_char == '\b' || rx_char == 0x7F)
            {
                if (line_len > 0)
                {
                    line_len--;
                    const char *erase_seq = "\b \b";
                    (void)uart_write_sync(uart, (const uint8_t *)erase_seq, strlen(erase_seq), pdMS_TO_TICKS(UART_EX_TIMEOUT_MS), NULL);
                }
            }
            /* Regular printable characters */
            else
            {
                if (line_len < (UART_EX_LINE_BUF_SIZE - 1U))
                {
                    line_buf[line_len++] = (char)rx_char;
                }
                /* Echo the character so it appears on screen as you type */
                (void)uart_write_sync(uart, &rx_char, 1U, pdMS_TO_TICKS(UART_EX_TIMEOUT_MS), NULL);
            }
        }
    }
}

void uart_example_start(void)
{
    (void)xTaskCreate(
        vUartExampleTask,
        "UartExample",
        256,
        NULL,
        2,
        NULL
    );
}
