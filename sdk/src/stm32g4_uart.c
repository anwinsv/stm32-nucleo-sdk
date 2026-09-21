/**
 * @file stm32g4_uart.c
 * @brief Hardware-abstraction implementation for STM32G4 UART instances.
 */
#include "stm32g4_uart.h"
#include "stm32g4xx.h"

extern uint32_t SystemCoreClock;

struct uart_hw_config {
    USART_TypeDef *regs;
    GPIO_TypeDef *tx_port;
    uint8_t tx_pin;
    uint8_t tx_af;
    GPIO_TypeDef *rx_port;
    uint8_t rx_pin;
    uint8_t rx_af;
};

static const struct uart_hw_config hw_configs[UART_INSTANCE_COUNT] = {
    [UART_INSTANCE_USART1] = {USART1, GPIOC, 4, 7, GPIOC, 5, 7},
    [UART_INSTANCE_USART2] = {USART2, GPIOA, 2, 7, GPIOA, 3, 7},
    [UART_INSTANCE_USART3] = {USART3, GPIOB, 10, 7, GPIOB, 11, 7},
    [UART_INSTANCE_UART4]  = {UART4,  GPIOC, 10, 5, GPIOC, 11, 5},
    [UART_INSTANCE_UART5]  = {UART5,  GPIOC, 12, 5, GPIOD, 2, 5},
    [UART_INSTANCE_LPUART1]= {LPUART1,GPIOA, 2, 12, GPIOA, 3, 12}
};

static void uart_enable_clocks(uart_instance_t instance) {
    switch(instance) {
        case UART_INSTANCE_USART1:
            RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
            RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
            break;
        case UART_INSTANCE_USART2:
            RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;
            RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
            break;
        case UART_INSTANCE_USART3:
            RCC->APB1ENR1 |= RCC_APB1ENR1_USART3EN;
            RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
            break;
        case UART_INSTANCE_UART4:
            RCC->APB1ENR1 |= RCC_APB1ENR1_UART4EN;
            RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
            break;
        case UART_INSTANCE_UART5:
            RCC->APB1ENR1 |= RCC_APB1ENR1_UART5EN;
            RCC->AHB2ENR |= (RCC_AHB2ENR_GPIOCEN | RCC_AHB2ENR_GPIODEN);
            break;
        case UART_INSTANCE_LPUART1:
            RCC->APB1ENR2 |= RCC_APB1ENR2_LPUART1EN;
            RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
            break;
        default: break;
    }
}

static void gpio_init_af(GPIO_TypeDef *port, uint8_t pin, uint8_t af) {
    port->MODER &= ~(3U << (pin * 2));
    port->MODER |= (2U << (pin * 2));
    port->OSPEEDR |= (3U << (pin * 2));
    port->PUPDR &= ~(3U << (pin * 2));
    port->PUPDR |= (1U << (pin * 2));
    if (pin < 8) {
        port->AFR[0] &= ~(0xFU << (pin * 4));
        port->AFR[0] |= ((uint32_t)af << (pin * 4));
    } else {
        port->AFR[1] &= ~(0xFU << ((pin - 8) * 4));
        port->AFR[1] |= ((uint32_t)af << ((pin - 8) * 4));
    }
}

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

static uint32_t uart_get_clock_freq(uart_instance_t instance) {
    uint32_t sel = 0;
    uint32_t pclk = 0;
    
    switch (instance) {
        case UART_INSTANCE_USART1:
            sel = (RCC->CCIPR & RCC_CCIPR_USART1SEL) >> RCC_CCIPR_USART1SEL_Pos;
            pclk = get_pclk2_freq();
            break;
        case UART_INSTANCE_USART2:
            sel = (RCC->CCIPR & RCC_CCIPR_USART2SEL) >> RCC_CCIPR_USART2SEL_Pos;
            pclk = get_pclk1_freq();
            break;
        case UART_INSTANCE_USART3:
            sel = (RCC->CCIPR & RCC_CCIPR_USART3SEL) >> RCC_CCIPR_USART3SEL_Pos;
            pclk = get_pclk1_freq();
            break;
        case UART_INSTANCE_UART4:
            sel = (RCC->CCIPR & RCC_CCIPR_UART4SEL) >> RCC_CCIPR_UART4SEL_Pos;
            pclk = get_pclk1_freq();
            break;
        case UART_INSTANCE_UART5:
            sel = (RCC->CCIPR & RCC_CCIPR_UART5SEL) >> RCC_CCIPR_UART5SEL_Pos;
            pclk = get_pclk1_freq();
            break;
        case UART_INSTANCE_LPUART1:
            sel = (RCC->CCIPR & RCC_CCIPR_LPUART1SEL) >> RCC_CCIPR_LPUART1SEL_Pos;
            pclk = get_pclk1_freq();
            break;
        default: return 0;
    }
    
    switch (sel) {
        case 0: return pclk;
        case 1: return SystemCoreClock;
        case 2: return 16000000U;
        case 3: return 32768U;
        default: return 0;
    }
}

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

        uart_enable_clocks(instance);

        const struct uart_hw_config *hw = &hw_configs[instance];
        gpio_init_af(hw->tx_port, hw->tx_pin, hw->tx_af);
        gpio_init_af(hw->rx_port, hw->rx_pin, hw->rx_af);
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

    USART_TypeDef *regs = hw_configs[uart->instance].regs;
    
    // Disable UART before configuration
    regs->CR1 &= ~USART_CR1_UE;

    // Word Length: M0 (bit 12), M1 (bit 28)
    regs->CR1 &= ~(USART_CR1_M0 | USART_CR1_M1);
    if (config->word_length == UART_WORD_LENGTH_7) {
        regs->CR1 |= USART_CR1_M1;
    } else if (config->word_length == UART_WORD_LENGTH_9) {
        regs->CR1 |= USART_CR1_M0;
    }

    // Parity
    regs->CR1 &= ~(USART_CR1_PCE | USART_CR1_PS);
    if (config->parity == UART_PARITY_EVEN) {
        regs->CR1 |= USART_CR1_PCE;
    } else if (config->parity == UART_PARITY_ODD) {
        regs->CR1 |= (USART_CR1_PCE | USART_CR1_PS);
    }

    // Stop bits
    regs->CR2 &= ~USART_CR2_STOP;
    if (config->stop_bits == UART_STOP_BITS_2) {
        regs->CR2 |= (2U << USART_CR2_STOP_Pos);
    }

    // Oversampling
    regs->CR1 &= ~USART_CR1_OVER8;
    if (config->oversampling == UART_OVERSAMPLING_8) {
        regs->CR1 |= USART_CR1_OVER8;
    }

    // Baud Rate
    uint32_t freq = uart_get_clock_freq(uart->instance);
    if (uart->instance == UART_INSTANCE_LPUART1) {
        // LPUART BRR = 256 * fCK / baudrate
        regs->BRR = (uint32_t)((256ULL * freq) / config->baudrate);
    } else {
        // USART BRR
        if (config->oversampling == UART_OVERSAMPLING_8) {
            uint32_t usartdiv = (2U * freq) / config->baudrate;
            regs->BRR = ((usartdiv & 0xFFF0U) | ((usartdiv & 0x000FU) >> 1));
        } else {
            regs->BRR = freq / config->baudrate;
        }
    }

    // FIFO Mode
    regs->CR1 &= ~USART_CR1_FIFOEN;
    if (config->fifo_mode == UART_FIFO_ENABLED) {
        regs->CR1 |= USART_CR1_FIFOEN;
    }

    // Flow Control
    regs->CR3 &= ~(USART_CR3_RTSE | USART_CR3_CTSE);
    if (config->flow_control == UART_FLOW_CONTROL_RTS || config->flow_control == UART_FLOW_CONTROL_RTS_CTS) {
        regs->CR3 |= USART_CR3_RTSE;
    }
    if (config->flow_control == UART_FLOW_CONTROL_CTS || config->flow_control == UART_FLOW_CONTROL_RTS_CTS) {
        regs->CR3 |= USART_CR3_CTSE;
    }

    // Mode (TX/RX Enable)
    regs->CR1 &= ~(USART_CR1_TE | USART_CR1_RE);
    if (uart_mode_has_tx(config->mode)) {
        regs->CR1 |= USART_CR1_TE;
    }
    if (uart_mode_has_rx(config->mode)) {
        regs->CR1 |= USART_CR1_RE;
    }

    // Enable UART
    regs->CR1 |= USART_CR1_UE;

    return UART_OK;
}

uart_status uart_write_polling(
    uart_handle_t uart,
    const uint8_t *buffer,
    size_t length,
    size_t *written)
{
    uart_status status;

    if (written != NULL)
    {
        *written = 0U;
    }

    status = uart_validate_transfer(uart, buffer, length, 1U);
    return (status == UART_OK) ? UART_NOT_SUPPORTED : status;
}

uart_status uart_read_polling(
    uart_handle_t uart,
    uint8_t *buffer,
    size_t length,
    size_t *read)
{
    uart_status status;

    if (read != NULL)
    {
        *read = 0U;
    }

    status = uart_validate_transfer(uart, buffer, length, 0U);
    return (status == UART_OK) ? UART_NOT_SUPPORTED : status;
}

uart_status uart_write_interrupt(
    uart_handle_t uart,
    const uint8_t *buffer,
    size_t length,
    uint32_t timeout_ticks,
    size_t *written)
{
    uart_status status;

    (void)timeout_ticks;

    if (written != NULL)
    {
        *written = 0U;
    }

    status = uart_validate_transfer(uart, buffer, length, 1U);
    return (status == UART_OK) ? UART_NOT_SUPPORTED : status;
}

uart_status uart_read_interrupt(
    uart_handle_t uart,
    uint8_t *buffer,
    size_t length,
    uint32_t timeout_ticks,
    size_t *read)
{
    uart_status status;

    (void)timeout_ticks;

    if (read != NULL)
    {
        *read = 0U;
    }

    status = uart_validate_transfer(uart, buffer, length, 0U);
    return (status == UART_OK) ? UART_NOT_SUPPORTED : status;
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
    uart_reset_descriptor(uart, instance);

    return UART_OK;
}
