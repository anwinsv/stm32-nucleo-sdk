/**
 * @file stm32g4_uart_ll.c
 * @brief Low-level register implementation for STM32G4 USART/UART/LPUART.
 *
 * All RCC, GPIO, and USART register accesses live here so that the HAL layer
 * (stm32g4_uart.c) remains hardware-agnostic.
 */
#include "stm32g4_uart_ll.h"
#include "stm32g4xx.h"

extern uint32_t SystemCoreClock;

/* ───────────────────────── Hardware pin/peripheral map ───────────────────── */

struct uart_hw_config {
    USART_TypeDef *regs;
    GPIO_TypeDef  *tx_port;
    uint8_t        tx_pin;
    uint8_t        tx_af;
    GPIO_TypeDef  *rx_port;
    uint8_t        rx_pin;
    uint8_t        rx_af;
};

static const struct uart_hw_config hw_configs[UART_INSTANCE_COUNT] = {
    [UART_INSTANCE_USART1]  = {USART1,  GPIOC, 4,  7,  GPIOC, 5,  7 },
    [UART_INSTANCE_USART2]  = {USART2,  GPIOD, 5,  7,  GPIOD, 6,  7 },
    [UART_INSTANCE_USART3]  = {USART3,  GPIOB, 10, 7,  GPIOB, 11, 7 },
    [UART_INSTANCE_UART4]   = {UART4,   GPIOC, 10, 5,  GPIOC, 11, 5 },
    [UART_INSTANCE_UART5]   = {UART5,   GPIOC, 12, 5,  GPIOD, 2,  5 },
    [UART_INSTANCE_LPUART1] = {LPUART1, GPIOA, 2,  12, GPIOA, 3,  12}
};

/* ─────────────────────────── Clock helpers ────────────────────────────────── */

static uint32_t get_pclk1_freq(void) {
    const uint8_t apb_presc[8] = {1, 1, 1, 1, 2, 4, 8, 16};
    uint32_t ppre1 = (RCC->CFGR & RCC_CFGR_PPRE1) >> RCC_CFGR_PPRE1_Pos;
    return SystemCoreClock / apb_presc[ppre1];
}

static uint32_t get_pclk2_freq(void) {
    const uint8_t apb_presc[8] = {1, 1, 1, 1, 2, 4, 8, 16};
    uint32_t ppre2 = (RCC->CFGR & RCC_CFGR_PPRE2) >> RCC_CFGR_PPRE2_Pos;
    return SystemCoreClock / apb_presc[ppre2];
}

static uint32_t uart_ll_get_clock_freq(uart_instance_t instance) {
    uint32_t sel  = 0;
    uint32_t pclk = 0;

    switch (instance) {
        case UART_INSTANCE_USART1:
            sel  = (RCC->CCIPR & RCC_CCIPR_USART1SEL) >> RCC_CCIPR_USART1SEL_Pos;
            pclk = get_pclk2_freq();
            break;
        case UART_INSTANCE_USART2:
            sel  = (RCC->CCIPR & RCC_CCIPR_USART2SEL) >> RCC_CCIPR_USART2SEL_Pos;
            pclk = get_pclk1_freq();
            break;
        case UART_INSTANCE_USART3:
            sel  = (RCC->CCIPR & RCC_CCIPR_USART3SEL) >> RCC_CCIPR_USART3SEL_Pos;
            pclk = get_pclk1_freq();
            break;
        case UART_INSTANCE_UART4:
            sel  = (RCC->CCIPR & RCC_CCIPR_UART4SEL) >> RCC_CCIPR_UART4SEL_Pos;
            pclk = get_pclk1_freq();
            break;
        case UART_INSTANCE_UART5:
            sel  = (RCC->CCIPR & RCC_CCIPR_UART5SEL) >> RCC_CCIPR_UART5SEL_Pos;
            pclk = get_pclk1_freq();
            break;
        case UART_INSTANCE_LPUART1:
            sel  = (RCC->CCIPR & RCC_CCIPR_LPUART1SEL) >> RCC_CCIPR_LPUART1SEL_Pos;
            pclk = get_pclk1_freq();
            break;
        default: return 0;
    }

    switch (sel) {
        case 0: return pclk;
        case 1: return SystemCoreClock;
        case 2: return 16000000U;      /* HSI16 */
        case 3: return 32768U;         /* LSE   */
        default: return 0;
    }
}

/* ─────────────────────────── GPIO helper ──────────────────────────────────── */

static void gpio_init_af(GPIO_TypeDef *port, uint8_t pin, uint8_t af) {
    port->MODER  &= ~(3U << (pin * 2));
    port->MODER  |=  (2U << (pin * 2));          /* Alternate function mode */
    port->OSPEEDR |= (3U << (pin * 2));          /* Very-high speed         */
    port->PUPDR  &= ~(3U << (pin * 2));
    port->PUPDR  |=  (1U << (pin * 2));          /* Pull-up                 */
    if (pin < 8) {
        port->AFR[0] &= ~(0xFU << (pin * 4));
        port->AFR[0] |=  ((uint32_t)af << (pin * 4));
    } else {
        port->AFR[1] &= ~(0xFU << ((pin - 8) * 4));
        port->AFR[1] |=  ((uint32_t)af << ((pin - 8) * 4));
    }
}

/* ═══════════════════════ Public LL functions ══════════════════════════════ */

void uart_ll_enable_clocks(uart_instance_t instance) {
    switch (instance) {
        case UART_INSTANCE_USART1:
            RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
            RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
            break;
        case UART_INSTANCE_USART2:
            RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;
            RCC->AHB2ENR  |= RCC_AHB2ENR_GPIODEN;
            break;
        case UART_INSTANCE_USART3:
            RCC->APB1ENR1 |= RCC_APB1ENR1_USART3EN;
            RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOBEN;
            break;
        case UART_INSTANCE_UART4:
            RCC->APB1ENR1 |= RCC_APB1ENR1_UART4EN;
            RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOCEN;
            break;
        case UART_INSTANCE_UART5:
            RCC->APB1ENR1 |= RCC_APB1ENR1_UART5EN;
            RCC->AHB2ENR  |= (RCC_AHB2ENR_GPIOCEN | RCC_AHB2ENR_GPIODEN);
            break;
        case UART_INSTANCE_LPUART1:
            RCC->APB1ENR2 |= RCC_APB1ENR2_LPUART1EN;
            RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOAEN;
            break;
        default: break;
    }
}

void uart_ll_init_gpio(uart_instance_t instance) {
    const struct uart_hw_config *hw = &hw_configs[instance];
    gpio_init_af(hw->tx_port, hw->tx_pin, hw->tx_af);
    gpio_init_af(hw->rx_port, hw->rx_pin, hw->rx_af);
}

uart_status uart_ll_configure(uart_instance_t instance,
                              const uart_config_t *config) {
    USART_TypeDef *regs = hw_configs[instance].regs;

    /* Disable UART before configuration */
    regs->CR1 &= ~USART_CR1_UE;

    /* Word Length: M0 (bit 12), M1 (bit 28) */
    regs->CR1 &= ~(USART_CR1_M0 | USART_CR1_M1);
    if (config->word_length == UART_WORD_LENGTH_7) {
        regs->CR1 |= USART_CR1_M1;
    } else if (config->word_length == UART_WORD_LENGTH_9) {
        regs->CR1 |= USART_CR1_M0;
    }

    /* Parity */
    regs->CR1 &= ~(USART_CR1_PCE | USART_CR1_PS);
    if (config->parity == UART_PARITY_EVEN) {
        regs->CR1 |= USART_CR1_PCE;
    } else if (config->parity == UART_PARITY_ODD) {
        regs->CR1 |= (USART_CR1_PCE | USART_CR1_PS);
    }

    /* Stop bits */
    regs->CR2 &= ~USART_CR2_STOP;
    if (config->stop_bits == UART_STOP_BITS_2) {
        regs->CR2 |= (2U << USART_CR2_STOP_Pos);
    }

    /* Oversampling (LPUART does not support OVER8; it uses a fixed 256x factor) */
    regs->CR1 &= ~USART_CR1_OVER8;
    if (config->oversampling == UART_OVERSAMPLING_8) {
        if (instance == UART_INSTANCE_LPUART1) {
            regs->CR1 |= USART_CR1_UE;
            return UART_INVALID_PARAM;
        }
        regs->CR1 |= USART_CR1_OVER8;
    }

    /* Baud Rate */
    uint32_t freq = uart_ll_get_clock_freq(instance);
    if (instance == UART_INSTANCE_LPUART1) {
        /* LPUART BRR = 256 * fCK / baudrate  (valid range: 0x300 .. 0xFFFFF) */
        uint32_t lpuart_brr = (uint32_t)((256ULL * freq) / config->baudrate);
        if ((lpuart_brr < 0x300U) || (lpuart_brr > 0xFFFFFU)) {
            regs->CR1 |= USART_CR1_UE;
            return UART_INVALID_PARAM;
        }
        regs->BRR = lpuart_brr;
    } else {
        uint32_t brr;
        if (config->oversampling == UART_OVERSAMPLING_8) {
            uint32_t usartdiv = (2U * freq) / config->baudrate;
            brr = ((usartdiv & 0xFFF0U) | ((usartdiv & 0x000FU) >> 1));
        } else {
            brr = freq / config->baudrate;
        }
        if (brr == 0U) {
            regs->CR1 |= USART_CR1_UE;
            return UART_INVALID_PARAM;
        }
        regs->BRR = brr;
    }

    /* FIFO Mode */
    regs->CR1 &= ~USART_CR1_FIFOEN;
    if (config->fifo_mode == UART_FIFO_ENABLED) {
        regs->CR1 |= USART_CR1_FIFOEN;
    }

    /* Flow Control */
    regs->CR3 &= ~(USART_CR3_RTSE | USART_CR3_CTSE);
    if ((config->flow_control == UART_FLOW_CONTROL_RTS) ||
        (config->flow_control == UART_FLOW_CONTROL_RTS_CTS)) {
        regs->CR3 |= USART_CR3_RTSE;
    }
    if ((config->flow_control == UART_FLOW_CONTROL_CTS) ||
        (config->flow_control == UART_FLOW_CONTROL_RTS_CTS)) {
        regs->CR3 |= USART_CR3_CTSE;
    }

    /* Mode (TX/RX Enable) */
    regs->CR1 &= ~(USART_CR1_TE | USART_CR1_RE);
    if ((config->mode == UART_MODE_TX) || (config->mode == UART_MODE_TX_RX)) {
        regs->CR1 |= USART_CR1_TE;
    }
    if ((config->mode == UART_MODE_RX) || (config->mode == UART_MODE_TX_RX)) {
        regs->CR1 |= USART_CR1_RE;
    }

    /* Enable UART */
    regs->CR1 |= USART_CR1_UE;

    return UART_OK;
}

void uart_ll_disable(uart_instance_t instance) {
    hw_configs[instance].regs->CR1 &= ~USART_CR1_UE;
}

uint8_t uart_ll_is_tx_empty(uart_instance_t instance) {
    return (hw_configs[instance].regs->ISR & USART_ISR_TXE) ? 1U : 0U;
}

void uart_ll_write_byte(uart_instance_t instance, uint8_t byte) {
    hw_configs[instance].regs->TDR = byte;
}

uint8_t uart_ll_is_tx_complete(uart_instance_t instance) {
    return (hw_configs[instance].regs->ISR & USART_ISR_TC) ? 1U : 0U;
}

uint8_t uart_ll_is_rx_ready(uart_instance_t instance) {
    return (hw_configs[instance].regs->ISR & USART_ISR_RXNE) ? 1U : 0U;
}

uint8_t uart_ll_read_byte(uart_instance_t instance) {
    return (uint8_t)(hw_configs[instance].regs->RDR & 0xFFU);
}

void uart_ll_enable_tx_interrupt(uart_instance_t instance) {
    hw_configs[instance].regs->CR1 |= USART_CR1_TXEIE;
}

void uart_ll_disable_tx_interrupt(uart_instance_t instance) {
    hw_configs[instance].regs->CR1 &= ~USART_CR1_TXEIE;
}

void uart_ll_enable_tc_interrupt(uart_instance_t instance) {
    hw_configs[instance].regs->CR1 |= USART_CR1_TCIE;
}

void uart_ll_disable_tc_interrupt(uart_instance_t instance) {
    hw_configs[instance].regs->CR1 &= ~USART_CR1_TCIE;
}

void uart_ll_enable_rx_interrupt(uart_instance_t instance) {
    hw_configs[instance].regs->CR1 |= USART_CR1_RXNEIE;
}

void uart_ll_disable_rx_interrupt(uart_instance_t instance) {
    hw_configs[instance].regs->CR1 &= ~USART_CR1_RXNEIE;
}

uint8_t uart_ll_is_tx_interrupt_enabled(uart_instance_t instance) {
    return (hw_configs[instance].regs->CR1 & USART_CR1_TXEIE) ? 1U : 0U;
}

uint8_t uart_ll_is_tc_interrupt_enabled(uart_instance_t instance) {
    return (hw_configs[instance].regs->CR1 & USART_CR1_TCIE) ? 1U : 0U;
}

uint8_t uart_ll_is_rx_interrupt_enabled(uart_instance_t instance) {
    return (hw_configs[instance].regs->CR1 & USART_CR1_RXNEIE) ? 1U : 0U;
}
