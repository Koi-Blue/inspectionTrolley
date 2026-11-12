#ifndef PID_H
#define PID_H

#include <stdint.h>

typedef struct {
    float kp;
    float ki;
    float kd;
    float integral;
    float prev_error;
    float out_min;
    float out_max;
    uint8_t has_prev;
} Pid;

void Pid_Init(Pid *pid, float kp, float ki, float kd, float out_min, float out_max);
void Pid_Reset(Pid *pid);
float Pid_Step(Pid *pid, float error, float dt);

#endif
