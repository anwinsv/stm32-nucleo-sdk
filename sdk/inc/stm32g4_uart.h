/**
 * @brief APIs for UART
 */
#include <stdint.h>

/* uart instance status */
typedef enum
{
    UART_OPEN,
    UART_CLOSED,
} open_status;

typedef enum
{
    UART_OK,
    UART_TX_BUSY,
    UART_RX_BUSY,
    UART_ERROR,
} uart_status;

/* uart descriptor */
typedef struct 
{
    uint32_t instance;
    uint32_t base_address;
    enum open_status is_open;
    enum uart_status status;
    uint32_t baudrate;
    uint8_t *tx_buffer;
    uint32_t tx_size;
    uint8_t *rx_buffer;
    uint32_t rx_size;
} uart_descriptor;

open_status uart_open(struct uart_descriptor *uart);
uart_status uart_ioctl(struct uart_descriptor *uart, uint32_t cmd, void *arg);
uart_status uart_write_sync(struct uart_descriptor *uart, uint8_t *buffer, uint32_t size);
uart_status uart_write_async(struct uart_descriptor *uart, uint8_t *buffer, uint32_t size);
uart_status uart_read_sync(struct uart_descriptor *uart, uint8_t *buffer, uint32_t size);
uart_status uart_read_async(struct uart_descriptor *uart, uint8_t *buffer, uint32_t size);
uart_status uart_close(struct uart_descriptor *uart);