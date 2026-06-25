//
// Created by tomoya on 2026/04/21.
//

#ifndef MODULARCANLIB_PID_H
#define MODULARCANLIB_PID_H

#include <stdbool.h>

typedef struct
{
    float kp, ki, kd, kff; // ゲイン
    float _integral;
    float _prev_value;
} ModularCANLib_PidParam_Type;

/**
 * @brief PIDパラメータの内部状態を初期化します
 *
 * @param pid_param 初期化対象のPIDパラメータ構造体へのポインタ
 */
void ModularCANLib_Pid_Init(ModularCANLib_PidParam_Type* pid_param);

/**
 * @brief PID制御計算を実行します
 * * @param pid_param         PIDパラメータ構造体へのポインタ
 * @param value_diff        現在の偏差 (目標値 - 現在値)
 * @param anti_windup_enable アンチワインドアップ（積分飽和抑制）を有効にするか
 * @param max_value         出力の最大値（絶対値）
 * @param update_freq       更新周波数 [Hz] (積分時間の計算に使用)
 *
 * @return float クリップ済みの制御出力
 */
float ModularCANLib_Pid_Ctrl(ModularCANLib_PidParam_Type* pid_param, float value_diff, bool anti_windup_enable, float max_value, float update_freq);

#endif //MODULARCANLIB_PID_H
