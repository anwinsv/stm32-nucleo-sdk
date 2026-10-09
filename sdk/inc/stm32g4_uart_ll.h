/**
 * @file stm32g4_uart_ll.h
 * @brief Low-level register interface for STM32G4 USART/UART/LPUART peripherals.
 *
 * This layer owns all direct hardware access: RCC clock gating, GPIO alternate-
 * function setup, and USART/LPUART register configuration.  It is consumed
 * exclusively by the HAL layer in stm32g4_uart.c.
 */
#ifndef STM32G4_UART_LL_H
#define STM32G4_UART_LL_H

#include "stm32g4_uart.h"

/**
 * Enable the peripheral and GPIO port clocks required by the given instance.
 */
void uart_ll_enable_clocks(uart_instance_t instance);

/**
 * Configure the TX and RX GPIO pins in alternate-function mode for the
 * given UART instance.
 */
void uart_ll_init_gpio(uart_instance_t instance);

/**
 * Apply a full set of communication settings to the USART/LPUART registers.
 *
 * The function disables the peripheral (UE = 0), programs CR1/CR2/CR3/BRR,
 * then re-enables it.  Returns UART_INVALID_PARAM when:
 *   - OVER8 is requested for LPUART1 (fixed 256x oversampling),
 *   - the computed BRR falls outside the valid hardware range.
 */
uart_status uart_ll_configure(uart_instance_t instance,
                              const uart_config_t *config);

/**
 * Disable the USART/LPUART peripheral (clear UE bit in CR1).
 */
void uart_ll_disable(uart_instance_t instance);

/**
 * Check if the transmit data register is empty (TXE).
 */
uint8_t uart_ll_is_tx_empty(uart_instance_t instance);

/**
 * Write a single byte to the transmit data register.
 */
void uart_ll_write_byte(uart_instance_t instance, uint8_t byte);

/**
 * Check if the transmission is complete (TC).
 */
uint8_t uart_ll_is_tx_complete(uart_instance_t instance);

/**
 * Check if the receive data register is not empty (RXNE).
 */
uint8_t uart_ll_is_rx_ready(uart_instance_t instance);

/**
 * Read a single byte from the receive data register.
 */
uint8_t uart_ll_read_byte(uart_instance_t instance);

/**
 * Enable/Disable TXE (Transmit Data Register Empty) interrupt.
 */
void uart_ll_enable_tx_interrupt(uart_instance_t instance);
void uart_ll_disable_tx_interrupt(uart_instance_t instance);

/**
 * Enable/Disable TC (Transmission Complete) interrupt.
 */
void uart_ll_enable_tc_interrupt(uart_instance_t instance);
void uart_ll_disable_tc_interrupt(uart_instance_t instance);

/**
 * Enable/Disable RXNE (Receive Data Register Not Empty) interrupt.
 */
void uart_ll_enable_rx_interrupt(uart_instance_t instance);
void uart_ll_disable_rx_interrupt(uart_instance_t instance);

/**
 * Check if specific interrupts are enabled in CR1.
 */
uint8_t uart_ll_is_tx_interrupt_enabled(uart_instance_t instance);
uint8_t uart_ll_is_tc_interrupt_enabled(uart_instance_t instance);
uint8_t uart_ll_is_rx_interrupt_enabled(uart_instance_t instance);

#endif /* STM32G4_UART_LL_H */
