//
// Created by tomoya on 2026/04/21.
//

#include "ModularCANLib_Pid.h"

#include <math.h>
#include <stdbool.h>

static float clip_f(const float x, const float min, const float max)
{
    return fminf(max, fmaxf(x, min));
}

void ModularCANLib_Pid_Init(ModularCANLib_PidParam_Type* pid_param)
{
    pid_param->_integral = 0;
    pid_param->_prev_value = 0;
}

float ModularCANLib_Pid_Ctrl(ModularCANLib_PidParam_Type* pid_param, const float value_diff, const bool anti_windup_enable, const float max_value, const float update_freq)
{
    const float integral = pid_param->_integral + (value_diff + (pid_param->_prev_value)) / 2.0f / update_freq; // 積分(台形近似)
    const float diff = (value_diff - pid_param->_prev_value);  // 差分
    const float ans = value_diff * pid_param->kp + integral * pid_param->ki + diff * pid_param->kd + pid_param->kff;
    if(anti_windup_enable && fabsf(ans) > max_value){
        pid_param->_integral += (0.0f + pid_param->_prev_value) / 2.0f / update_freq;
    }else{
        pid_param->_integral = integral;
    };
    pid_param->_prev_value = value_diff;
    return clip_f(ans, -max_value, max_value);
}