#ifndef BOARD_MOTOR_H
#define BOARD_MOTOR_H

#include "motor.h"

#define BOARD_MOTOR_COUNT 4u

extern Motor g_board_motor[BOARD_MOTOR_COUNT];

void BoardMotor_Init(void);

#endif
