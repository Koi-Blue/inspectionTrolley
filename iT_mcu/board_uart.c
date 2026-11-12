#include "board_uart.h"

#include "stm32f4xx_gpio.h"
#include "stm32f4xx_usart.h"

/* CH340C: its RXD is PD8 (USART3_TX), its TXD is PD9 (USART3_RX). */
static const UartConfig s_cfg = {
    .tx_port = GPIOD,
    .tx_pin = GPIO_Pin_8,
    .tx_source = GPIO_PinSource8,
    .tx_alternate = GPIO_AF_USART3,
    .rx_port = GPIOD,
    .rx_pin = GPIO_Pin_9,
    .rx_source = GPIO_PinSource9,
    .rx_alternate = GPIO_AF_USART3,
    .usart = USART3,
    .baud = 115200u
};

Uart g_board_uart;

void BoardUart_Init(void)
{
    Uart_Init(&g_board_uart, &s_cfg);
}
