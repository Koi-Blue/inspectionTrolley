#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>

#include "stm32f4xx.h"

typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
    uint8_t source;
    uint8_t alternate;
} MotorPin;

typedef struct {
    MotorPin in1;
    MotorPin in2;
    TIM_TypeDef *pwm_tim;
    volatile uint32_t *ccr_in1;
    volatile uint32_t *ccr_in2;
    MotorPin enc_b;
    MotorPin enc_a;
    TIM_TypeDef *enc_tim;
    IRQn_Type enc_irq;
    uint32_t pwm_hz;
    int8_t count_sign;
} MotorConfig;

typedef struct Motor {
    const MotorConfig *cfg;
    volatile int32_t count_base;
    uint32_t pwm_top;
} Motor;

void Motor_Init(Motor *motor, const MotorConfig *cfg);
void Motor_Set(Motor *motor, int16_t permille);
void Motor_Coast(Motor *motor);
void Motor_Brake(Motor *motor);
int32_t Motor_GetCount(Motor *motor);
void Motor_ResetCount(Motor *motor);

#endif
