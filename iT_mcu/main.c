#include "RTE_Components.h"
#include CMSIS_device_header

#include <stdio.h>

#include "board_motor.h"
#include "board_uart.h"
#include "pid.h"

#define SPEED_KP 0.30f
#define SPEED_KI 0.05f
#define SPEED_KD 0.0f
#define SPEED_TARGET_COUNTS 680.0f
#define SPEED_DT_S 0.01f
#define PRINT_EVERY_TICKS 10u

static volatile uint32_t s_tick_10ms;

void SysTick_Handler(void)
{
    s_tick_10ms++;
}

int main(void)
{
    Pid speed[BOARD_MOTOR_COUNT];
    int32_t last_count[BOARD_MOTOR_COUNT];
    int32_t last_measured[BOARD_MOTOR_COUNT];
    int16_t last_command[BOARD_MOTOR_COUNT];
    uint32_t seen_tick;
    uint32_t since_print;
    uint32_t i;

    BoardUart_Init();
    BoardMotor_Init();
    for (i = 0; i < BOARD_MOTOR_COUNT; i++) {
        Pid_Init(&speed[i], SPEED_KP, SPEED_KI, SPEED_KD, -1000.0f, 1000.0f);
        Motor_ResetCount(&g_board_motor[i]);
        last_count[i] = 0;
        last_measured[i] = 0;
        last_command[i] = 0;
    }

    SysTick_Config(SystemCoreClock / 100u);
    seen_tick = s_tick_10ms;
    since_print = 0u;

    for (;;) {
        uint32_t now = s_tick_10ms;
        uint32_t steps;

        if (now == seen_tick) {
            continue;
        }
        steps = now - seen_tick;
        seen_tick = now;
        since_print += steps;

        for (i = 0; i < BOARD_MOTOR_COUNT; i++) {
            int32_t count = Motor_GetCount(&g_board_motor[i]);
            float measured = (float)(count - last_count[i]) / (float)steps;
            float command;

            last_count[i] = count;
            last_measured[i] = (int32_t)measured;
            command = Pid_Step(&speed[i], SPEED_TARGET_COUNTS - measured,
                                SPEED_DT_S * (float)steps);
            last_command[i] = (int16_t)command;
            Motor_Set(&g_board_motor[i], last_command[i]);
        }

        if (since_print >= PRINT_EVERY_TICKS) {
            since_print = 0u;
            printf("A %ld %d B %ld %d C %ld %d D %ld %d\n",
                   (long)last_measured[0], (int)last_command[0],
                   (long)last_measured[1], (int)last_command[1],
                   (long)last_measured[2], (int)last_command[2],
                   (long)last_measured[3], (int)last_command[3]);
        }
    }
}
