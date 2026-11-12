#include "uart.h"

#include <stddef.h>

#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_usart.h"

static void enable_gpio_clock(GPIO_TypeDef *port)
{
    if (port == GPIOA) {
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    } else if (port == GPIOB) {
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    } else if (port == GPIOC) {
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    } else if (port == GPIOD) {
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
    }
}

static void enable_usart_clock(USART_TypeDef *usart)
{
    if (usart == USART1) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    } else if (usart == USART2) {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    } else if (usart == USART3) {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
    } else if (usart == USART6) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART6, ENABLE);
    }
}

static void init_pin(GPIO_TypeDef *port, uint16_t pin, uint8_t source, uint8_t alternate)
{
    GPIO_InitTypeDef gpio;

    enable_gpio_clock(port);
    GPIO_PinAFConfig(port, source, alternate);
    gpio.GPIO_Pin = pin;
    gpio.GPIO_Mode = GPIO_Mode_AF;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(port, &gpio);
}

void Uart_Init(Uart *uart, const UartConfig *cfg)
{
    USART_InitTypeDef usart;

    if (uart == NULL || cfg == NULL || cfg->usart == NULL || cfg->baud == 0u) {
        return;
    }

    uart->cfg = cfg;
    uart->ready = 0u;
    init_pin(cfg->tx_port, cfg->tx_pin, cfg->tx_source, cfg->tx_alternate);
    init_pin(cfg->rx_port, cfg->rx_pin, cfg->rx_source, cfg->rx_alternate);
    enable_usart_clock(cfg->usart);

    USART_StructInit(&usart);
    usart.USART_BaudRate = cfg->baud;
    usart.USART_WordLength = USART_WordLength_8b;
    usart.USART_StopBits = USART_StopBits_1;
    usart.USART_Parity = USART_Parity_No;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(cfg->usart, &usart);
    USART_Cmd(cfg->usart, ENABLE);
    uart->ready = 1u;
}

size_t Uart_Write(Uart *uart, const uint8_t *data, size_t len)
{
    size_t i;

    if (uart == NULL || uart->ready == 0u || data == NULL) {
        return 0u;
    }

    for (i = 0u; i < len; i++) {
        while (USART_GetFlagStatus(uart->cfg->usart, USART_FLAG_TXE) == RESET) {
        }
        USART_SendData(uart->cfg->usart, data[i]);
    }
    while (USART_GetFlagStatus(uart->cfg->usart, USART_FLAG_TC) == RESET) {
    }
    return len;
}

size_t Uart_Read(Uart *uart, uint8_t *data, size_t len)
{
    size_t i;

    if (uart == NULL || uart->ready == 0u || data == NULL || len == 0u) {
        return 0u;
    }

    while (USART_GetFlagStatus(uart->cfg->usart, USART_FLAG_RXNE) == RESET) {
    }
    data[0] = (uint8_t)USART_ReceiveData(uart->cfg->usart);
    for (i = 1u; i < len; i++) {
        if (USART_GetFlagStatus(uart->cfg->usart, USART_FLAG_RXNE) == RESET) {
            break;
        }
        data[i] = (uint8_t)USART_ReceiveData(uart->cfg->usart);
    }
    return i;
}
