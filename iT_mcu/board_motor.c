#include "board_motor.h"

#include "stm32f4xx_gpio.h"
#include "stm32f4xx_tim.h"

/*
 * Socket pin 1 is IN1, pin 6 is IN2, pin 3 is encoder B, pin 4 is encoder A.
 * count_sign is +1 until a positive Motor_Set makes Motor_GetCount decrease.
 * Motors that share a PWM timer must use the same pwm_hz.
 */
static const MotorConfig s_cfg[BOARD_MOTOR_COUNT] = {
    {
        .in1 = {GPIOA, GPIO_Pin_15, GPIO_PinSource15, GPIO_AF_TIM2},
        .in2 = {GPIOB, GPIO_Pin_3, GPIO_PinSource3, GPIO_AF_TIM2},
        .pwm_tim = TIM2,
        .ccr_in1 = &TIM2->CCR1,
        .ccr_in2 = &TIM2->CCR2,
        .enc_b = {GPIOD, GPIO_Pin_12, GPIO_PinSource12, GPIO_AF_TIM4},
        .enc_a = {GPIOD, GPIO_Pin_13, GPIO_PinSource13, GPIO_AF_TIM4},
        .enc_tim = TIM4,
        .enc_irq = TIM4_IRQn,
        .pwm_hz = 20000u,
        .count_sign = 1
    },
    {
        .in1 = {GPIOB, GPIO_Pin_10, GPIO_PinSource10, GPIO_AF_TIM2},
        .in2 = {GPIOB, GPIO_Pin_11, GPIO_PinSource11, GPIO_AF_TIM2},
        .pwm_tim = TIM2,
        .ccr_in1 = &TIM2->CCR3,
        .ccr_in2 = &TIM2->CCR4,
        .enc_b = {GPIOA, GPIO_Pin_0, GPIO_PinSource0, GPIO_AF_TIM5},
        .enc_a = {GPIOA, GPIO_Pin_1, GPIO_PinSource1, GPIO_AF_TIM5},
        .enc_tim = TIM5,
        .enc_irq = TIM5_IRQn,
        .pwm_hz = 20000u,
        .count_sign = 1
    },
    {
        .in1 = {GPIOA, GPIO_Pin_6, GPIO_PinSource6, GPIO_AF_TIM3},
        .in2 = {GPIOA, GPIO_Pin_7, GPIO_PinSource7, GPIO_AF_TIM3},
        .pwm_tim = TIM3,
        .ccr_in1 = &TIM3->CCR1,
        .ccr_in2 = &TIM3->CCR2,
        .enc_b = {GPIOE, GPIO_Pin_9, GPIO_PinSource9, GPIO_AF_TIM1},
        .enc_a = {GPIOE, GPIO_Pin_11, GPIO_PinSource11, GPIO_AF_TIM1},
        .enc_tim = TIM1,
        .enc_irq = TIM1_UP_TIM10_IRQn,
        .pwm_hz = 20000u,
        .count_sign = 1
    },
    {
        .in1 = {GPIOB, GPIO_Pin_0, GPIO_PinSource0, GPIO_AF_TIM3},
        .in2 = {GPIOB, GPIO_Pin_1, GPIO_PinSource1, GPIO_AF_TIM3},
        .pwm_tim = TIM3,
        .ccr_in1 = &TIM3->CCR3,
        .ccr_in2 = &TIM3->CCR4,
        .enc_b = {GPIOC, GPIO_Pin_6, GPIO_PinSource6, GPIO_AF_TIM8},
        .enc_a = {GPIOC, GPIO_Pin_7, GPIO_PinSource7, GPIO_AF_TIM8},
        .enc_tim = TIM8,
        .enc_irq = TIM8_UP_TIM13_IRQn,
        .pwm_hz = 20000u,
        .count_sign = 1
    }
};

Motor g_board_motor[BOARD_MOTOR_COUNT];

void BoardMotor_Init(void)
{
    uint32_t i;

    for (i = 0; i < BOARD_MOTOR_COUNT; i++) {
        Motor_Init(&g_board_motor[i], &s_cfg[i]);
    }
}
