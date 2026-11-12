#ifndef STM32F4XX_CONF_H
#define STM32F4XX_CONF_H

#include <stdint.h>
#include "stm32f4xx.h"

/* Board crystal is 8 MHz. The running clock stays on HSI until a later change. */
#if !defined(HSE_VALUE)
#define HSE_VALUE ((uint32_t)8000000)
#endif
#if !defined(HSI_VALUE)
#define HSI_VALUE ((uint32_t)16000000)
#endif
#if !defined(HSE_STARTUP_TIMEOUT)
#define HSE_STARTUP_TIMEOUT ((uint16_t)0x05000)
#endif

/*
 * The standard-library GPIO check names GPIOJ and GPIOK. The STM32F407
 * device header does not provide those ports.
 */
#if !defined(GPIOJ)
#define GPIOJ ((GPIO_TypeDef *)0)
#endif
#if !defined(GPIOK)
#define GPIOK ((GPIO_TypeDef *)0)
#endif

#ifdef USE_FULL_ASSERT
#define assert_param(expr) ((expr) ? (void)0 : assert_failed((uint8_t *)__FILE__, __LINE__))
void assert_failed(uint8_t *file, uint32_t line);
#else
#define assert_param(expr) ((void)0)
#endif

#endif
