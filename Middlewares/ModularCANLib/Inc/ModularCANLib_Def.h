//
// Created by tomoya on 2026/04/20.
//

#ifndef MODULARCANLIB_SETTING_H
#define MODULARCANLIB_SETTING_H

// ReSharper disable once CppUnusedIncludeDirective
#include "stdio.h" // マクロ置換printf用
#include "can.h"

// マクロ設定
// printfによるデバッグを有効化するとき、コメントを外す
#define ENABLE_MODULARCANLIB_PRINTF_DEBUG
// printfによる情報表示を有効化するとき、コメントを外す
#define ENABLE_MODULARCANLIB_PRINTF_INFO

// マクロ定義
#ifdef ENABLE_MODULARCANLIB_PRINTF_DEBUG
#define MODULARCANLIB_PRINTF_DEBUG(...) printf(__VA_ARGS__)
#else
#define MODULARCANLIB_PRINTF_DEBUG(...)
#endif

#ifdef ENABLE_MODULARCANLIB_PRINTF_INFO
#define MODULARCANLIB_PRINTF_INFO(...) printf(__VA_ARGS__)
#else
#define MODULARCANLIB_PRINTF_INFO(...)
#endif

typedef enum : uint8_t
{
    ModularCANLib_CAN_ID_TYPE_STD,
    ModularCANLib_CAN_ID_TYPE_EXT,
} ModularCANLib_CANId_Type;

// NUM_OF_CANS
#if defined(CAN3)
#define NUM_OF_CANS 3
#elif defined(CAN2)
#define NUM_OF_CANS 2
#elif defined(CAN1) || defined(CAN)
#define NUM_OF_CANS 1
#else
#error "CANが検出されませんでした。.iocで設定していますか？"
#endif

#endif //MODULARCANLIB_SETTING_H
