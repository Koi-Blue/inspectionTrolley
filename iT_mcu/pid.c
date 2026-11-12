#include "pid.h"

#include <stddef.h>
#include <stdint.h>

void Pid_Init(Pid *pid, float kp, float ki, float kd, float out_min, float out_max)
{
    if (pid == NULL) {
        return;
    }
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->out_min = out_min;
    pid->out_max = out_max;
    Pid_Reset(pid);
}

void Pid_Reset(Pid *pid)
{
    if (pid == NULL) {
        return;
    }
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
    pid->has_prev = 0u;
}

float Pid_Step(Pid *pid, float error, float dt)
{
    float derivative;
    float integral;
    float output;

    if (pid == NULL || dt <= 0.0f) {
        return 0.0f;
    }

    if (pid->has_prev == 0u) {
        derivative = 0.0f;
        pid->has_prev = 1u;
    } else {
        derivative = (error - pid->prev_error) / dt;
    }
    pid->prev_error = error;

    integral = pid->integral + error * dt;
    output = pid->kp * error + pid->ki * integral + pid->kd * derivative;
    if (output > pid->out_max) {
        output = pid->out_max;
    } else if (output < pid->out_min) {
        output = pid->out_min;
    } else {
        pid->integral = integral;
    }
    return output;
}
