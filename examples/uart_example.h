/**
 * @file    uart_example.h
 * @brief   UART and NVIC validation example for STM32G4.
 */

#ifndef UART_EXAMPLE_H
#define UART_EXAMPLE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Spawn the FreeRTOS UART example test task.
 *
 * Configures LPUART1 on the ST-LINK Virtual COM port (115200 8N1) and executes:
 *   1. Polling write sanity check (uart_write_polling)
 *   2. Interrupt-driven sync write banner (uart_write_sync)
 *   3. Interactive interrupt-driven echo loop (uart_read_sync + uart_write_sync)
 */
void uart_example_start(void);

#ifdef __cplusplus
}
#endif

#endif /* UART_EXAMPLE_H */
