#ifndef UART_H
#define UART_H

#include <stddef.h>
#include <stdint.h>

#include "stm32f4xx.h"

typedef struct {
    GPIO_TypeDef *tx_port;
    uint16_t tx_pin;
    uint8_t tx_source;
    uint8_t tx_alternate;
    GPIO_TypeDef *rx_port;
    uint16_t rx_pin;
    uint8_t rx_source;
    uint8_t rx_alternate;
    USART_TypeDef *usart;
    uint32_t baud;
} UartConfig;

typedef struct Uart {
    const UartConfig *cfg;
    uint8_t ready;
} Uart;

void Uart_Init(Uart *uart, const UartConfig *cfg);
size_t Uart_Write(Uart *uart, const uint8_t *data, size_t len);
size_t Uart_Read(Uart *uart, uint8_t *data, size_t len);

#endif
