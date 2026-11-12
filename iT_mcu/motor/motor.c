#include "motor.h"

#include <stddef.h>

#include "stm32f4xx_conf.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_tim.h"

#define ENCODER_PERIOD 65535u
#define ENCODER_MODULUS 65536
#define ENCODER_FILTER 2u
#define ENCODER_IRQ_PRIORITY 6u
#define MOTOR_SLOT_COUNT 4u

static Motor *s_bound[MOTOR_SLOT_COUNT];

static uint32_t timer_kernel_hz(TIM_TypeDef *tim)
{
    RCC_ClocksTypeDef clocks;
    uint32_t prescaler;

    RCC_GetClocksFreq(&clocks);
    if (tim == TIM1 || tim == TIM8 || tim == TIM9 || tim == TIM10 || tim == TIM11) {
        prescaler = (RCC->CFGR & RCC_CFGR_PPRE2) >> 13;
        if (prescaler >= 4u) {
            return clocks.PCLK2_Frequency * 2u;
        }
        return clocks.PCLK2_Frequency;
    }

    prescaler = (RCC->CFGR & RCC_CFGR_PPRE1) >> 10;
    if (prescaler >= 4u) {
        return clocks.PCLK1_Frequency * 2u;
    }
    return clocks.PCLK1_Frequency;
}

static void enable_gpio_clock(GPIO_TypeDef *port)
{
    uint32_t bit = 0;

    if (port == GPIOA) {
        bit = RCC_AHB1Periph_GPIOA;
    } else if (port == GPIOB) {
        bit = RCC_AHB1Periph_GPIOB;
    } else if (port == GPIOC) {
        bit = RCC_AHB1Periph_GPIOC;
    } else if (port == GPIOD) {
        bit = RCC_AHB1Periph_GPIOD;
    } else if (port == GPIOE) {
        bit = RCC_AHB1Periph_GPIOE;
    }

    if (bit != 0u) {
        RCC_AHB1PeriphClockCmd(bit, ENABLE);
    }
}

static void enable_tim_clock(TIM_TypeDef *tim)
{
    if (tim == TIM1) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
    } else if (tim == TIM2) {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    } else if (tim == TIM3) {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    } else if (tim == TIM4) {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
    } else if (tim == TIM5) {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);
    } else if (tim == TIM8) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM8, ENABLE);
    }
}

static void init_pin(const MotorPin *pin, GPIOPuPd_TypeDef pull)
{
    GPIO_InitTypeDef gpio;

    enable_gpio_clock(pin->port);
    GPIO_PinAFConfig(pin->port, pin->source, pin->alternate);
    gpio.GPIO_Pin = pin->pin;
    gpio.GPIO_Mode = GPIO_Mode_AF;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_PuPd = pull;
    GPIO_Init(pin->port, &gpio);
}

static void init_pwm_channel(TIM_TypeDef *tim, volatile uint32_t *ccr)
{
    TIM_OCInitTypeDef output;

    TIM_OCStructInit(&output);
    output.TIM_OCMode = TIM_OCMode_PWM1;
    output.TIM_OutputState = TIM_OutputState_Enable;
    output.TIM_Pulse = 0;
    output.TIM_OCPolarity = TIM_OCPolarity_High;

    if (ccr == &tim->CCR1) {
        TIM_OC1Init(tim, &output);
        TIM_OC1PreloadConfig(tim, TIM_OCPreload_Enable);
    } else if (ccr == &tim->CCR2) {
        TIM_OC2Init(tim, &output);
        TIM_OC2PreloadConfig(tim, TIM_OCPreload_Enable);
    } else if (ccr == &tim->CCR3) {
        TIM_OC3Init(tim, &output);
        TIM_OC3PreloadConfig(tim, TIM_OCPreload_Enable);
    } else if (ccr == &tim->CCR4) {
        TIM_OC4Init(tim, &output);
        TIM_OC4PreloadConfig(tim, TIM_OCPreload_Enable);
    }
}

static void init_pwm(Motor *motor)
{
    TIM_TypeDef *tim = motor->cfg->pwm_tim;
    TIM_TimeBaseInitTypeDef time_base;
    uint32_t tim_hz;
    uint16_t period;

    enable_tim_clock(tim);
    if ((tim->CR1 & TIM_CR1_CEN) == 0u && motor->cfg->pwm_hz >= 2u) {
        tim_hz = timer_kernel_hz(tim);
        period = (uint16_t)((tim_hz / motor->cfg->pwm_hz) - 1u);
        TIM_TimeBaseStructInit(&time_base);
        time_base.TIM_Period = period;
        time_base.TIM_Prescaler = 0;
        time_base.TIM_ClockDivision = TIM_CKD_DIV1;
        time_base.TIM_CounterMode = TIM_CounterMode_Up;
        TIM_TimeBaseInit(tim, &time_base);
        TIM_ARRPreloadConfig(tim, ENABLE);
        motor->pwm_top = (uint32_t)period + 1u;
        TIM_Cmd(tim, ENABLE);
    } else {
        motor->pwm_top = tim->ARR + 1u;
    }

    init_pwm_channel(tim, motor->cfg->ccr_in1);
    init_pwm_channel(tim, motor->cfg->ccr_in2);
}

static void init_encoder(Motor *motor)
{
    TIM_TypeDef *tim = motor->cfg->enc_tim;
    TIM_TimeBaseInitTypeDef time_base;

    enable_tim_clock(tim);
    TIM_TimeBaseStructInit(&time_base);
    time_base.TIM_Period = ENCODER_PERIOD;
    time_base.TIM_Prescaler = 0;
    time_base.TIM_ClockDivision = TIM_CKD_DIV1;
    time_base.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(tim, &time_base);
    TIM_EncoderInterfaceConfig(tim, TIM_EncoderMode_TI12,
                               TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);
    tim->CCMR1 = (tim->CCMR1 & (uint32_t)~(TIM_CCMR1_IC1F | TIM_CCMR1_IC2F)) |
                 (ENCODER_FILTER << TIM_CCMR1_IC1F_Pos) |
                 (ENCODER_FILTER << TIM_CCMR1_IC2F_Pos);
    TIM_SetCounter(tim, 0);
    TIM_Cmd(tim, ENABLE);
    TIM_ClearFlag(tim, TIM_FLAG_Update);
    TIM_ITConfig(tim, TIM_IT_Update, ENABLE);
    NVIC_SetPriority(motor->cfg->enc_irq, ENCODER_IRQ_PRIORITY);
    NVIC_EnableIRQ(motor->cfg->enc_irq);
}

static void bind_motor(Motor *motor)
{
    uint32_t i;

    for (i = 0; i < MOTOR_SLOT_COUNT; i++) {
        if (s_bound[i] == NULL || s_bound[i]->cfg->enc_tim == motor->cfg->enc_tim) {
            s_bound[i] = motor;
            return;
        }
    }
}

static Motor *motor_from_tim(TIM_TypeDef *tim)
{
    uint32_t i;

    for (i = 0; i < MOTOR_SLOT_COUNT; i++) {
        if (s_bound[i] != NULL && s_bound[i]->cfg->enc_tim == tim) {
            return s_bound[i];
        }
    }
    return NULL;
}

static void encoder_overflow(Motor *motor)
{
    TIM_TypeDef *tim;

    if (motor == NULL) {
        return;
    }
    tim = motor->cfg->enc_tim;
    if (TIM_GetITStatus(tim, TIM_IT_Update) == RESET) {
        return;
    }
    TIM_ClearITPendingBit(tim, TIM_IT_Update);
    if ((tim->CR1 & TIM_CR1_DIR) != 0u) {
        motor->count_base -= ENCODER_MODULUS;
    } else {
        motor->count_base += ENCODER_MODULUS;
    }
}

static uint32_t duty_from_permille(const Motor *motor, int16_t magnitude)
{
    return ((uint32_t)magnitude * motor->pwm_top) / 1000u;
}

void Motor_Init(Motor *motor, const MotorConfig *cfg)
{
    if (motor == NULL || cfg == NULL || cfg->pwm_tim == NULL || cfg->enc_tim == NULL) {
        return;
    }

    motor->cfg = cfg;
    motor->count_base = 0;
    motor->pwm_top = 1u;
    init_pin(&cfg->in1, GPIO_PuPd_DOWN);
    init_pin(&cfg->in2, GPIO_PuPd_DOWN);
    init_pin(&cfg->enc_b, GPIO_PuPd_UP);
    init_pin(&cfg->enc_a, GPIO_PuPd_UP);
    init_pwm(motor);
    init_encoder(motor);
    bind_motor(motor);
}

void Motor_Set(Motor *motor, int16_t permille)
{
    uint32_t duty;

    if (motor == NULL || motor->cfg == NULL) {
        return;
    }
    if (permille > 1000) {
        permille = 1000;
    } else if (permille < -1000) {
        permille = -1000;
    }

    if (permille == 0) {
        *motor->cfg->ccr_in1 = 0u;
        *motor->cfg->ccr_in2 = 0u;
        return;
    }

    duty = duty_from_permille(motor, (int16_t)(permille > 0 ? permille : (int16_t)-permille));
    if (permille > 0) {
        *motor->cfg->ccr_in1 = duty;
        *motor->cfg->ccr_in2 = 0u;
    } else {
        *motor->cfg->ccr_in1 = 0u;
        *motor->cfg->ccr_in2 = duty;
    }
}

void Motor_Coast(Motor *motor)
{
    Motor_Set(motor, 0);
}

void Motor_Brake(Motor *motor)
{
    if (motor == NULL || motor->cfg == NULL) {
        return;
    }
    *motor->cfg->ccr_in1 = motor->pwm_top;
    *motor->cfg->ccr_in2 = motor->pwm_top;
}

int32_t Motor_GetCount(Motor *motor)
{
    int32_t base;
    uint16_t count;
    int8_t sign;

    if (motor == NULL || motor->cfg == NULL) {
        return 0;
    }

    do {
        base = motor->count_base;
        count = (uint16_t)motor->cfg->enc_tim->CNT;
    } while (base != motor->count_base);

    sign = motor->cfg->count_sign;
    if (sign == 0) {
        sign = 1;
    }
    return (int32_t)sign * (base + (int32_t)count);
}

void Motor_ResetCount(Motor *motor)
{
    if (motor == NULL || motor->cfg == NULL) {
        return;
    }

    NVIC_DisableIRQ(motor->cfg->enc_irq);
    motor->cfg->enc_tim->CNT = 0u;
    motor->count_base = 0;
    TIM_ClearFlag(motor->cfg->enc_tim, TIM_FLAG_Update);
    NVIC_EnableIRQ(motor->cfg->enc_irq);
}

void TIM4_IRQHandler(void)
{
    encoder_overflow(motor_from_tim(TIM4));
}

void TIM5_IRQHandler(void)
{
    encoder_overflow(motor_from_tim(TIM5));
}

void TIM1_UP_TIM10_IRQHandler(void)
{
    encoder_overflow(motor_from_tim(TIM1));
}

void TIM8_UP_TIM13_IRQHandler(void)
{
    encoder_overflow(motor_from_tim(TIM8));
}
