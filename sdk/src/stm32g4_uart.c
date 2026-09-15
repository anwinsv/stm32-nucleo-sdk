/**
 * @brief HAL APIs for UART driver
 */

 /* includes */
 #include <stdio.h>
 #include "stm32g4_uart.h"

/* APIs */
open_status uart_open(struct uart_descriptor *uart)
{
    //set the uart instance to open

}

uart_status uart_ioctl(struct uart_descriptor *uart, uint32_t cmd, void *arg)
{
    //implement the uart ioctl function
}

uart_status uart_write_sync(struct uart_descriptor *uart, uint8_t *buffer, uint32_t size)
{
    //implement the uart write function - polling
}

uart_status uart_write_async(struct uart_descriptor *uart, uint8_t *buffer, uint32_t size)
{
    //implement the uart write function - interrupt
}

uart_status uart_read_sync(struct uart_descriptor *uart, uint8_t *buffer, uint32_t size)
{
    //implement the uart read function - polling
}

uart_status uart_read_async(struct uart_descriptor *uart, uint8_t *buffer, uint32_t size)
{
    //implement the uart read function - interrupt
}

uart_status uart_close(struct uart_descriptor *uart)
{
    //implement the uart close function
}

void uart_isr(struct uart_descriptor *uart)
{
    //implement the uart interrupt service routine
}