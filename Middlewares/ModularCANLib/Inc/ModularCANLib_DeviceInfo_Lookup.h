//
// Created by tomoya on 2026/04/21.
//

#ifndef SWERVE_CONTROL_BOARD_FIRMWARE_MODULARCANLIB_DEVICEINFO_LOOKUP_H
#define SWERVE_CONTROL_BOARD_FIRMWARE_MODULARCANLIB_DEVICEINFO_LOOKUP_H

#include "can.h"

#include "ModularCANLib_DeviceInfo_Def.h"

/**
 * @brief デバイス情報検索用のルックアップテーブルを構築する
 * * 全デバイスの情報を走査し、CANハンドラ別およびデバイスタイプ別の2つの検索テーブルを初期化します。
 * システム起動時など、通信を開始する前に一度呼び出す必要があります。2度呼び出してはいけません。
 * * @note 内部で保持している static なテーブル配列に情報を格納します。
 */
void ModularCANLib_MakeLookupTable(void);

/**
 * @brief 指定されたCANハンドラに紐付くデバイス情報の一覧を取得する
 * * @param[in]  hcan              検索対象となるCANハンドラへのポインタ
 * @param[out] device_info_ptrs  デバイス情報構造体へのポインタ配列の先頭アドレスを格納するポインタ
 * @param[out] size              見つかったデバイスの個数を格納する変数へのポインタ
 */
void ModularCANLib_GetDeviceInfos_By_Hcan(const CAN_HandleTypeDef *hcan, ModularCANLib_DeviceInfo_Type ***device_info_ptrs, uint32_t *size);

/**
 * @brief 指定されたデバイスタイプに該当するデバイス情報の一覧を取得する
 * * @param[in]  device_type       検索対象のデバイスタイプ
 * @param[out] device_info_ptrs  デバイス情報構造体へのポインタ配列の先頭アドレスを格納するポインタ
 * @param[out] size              見つかったデバイスの個数を格納する変数へのポインタ
 */
void ModularCANLib_GetDeviceInfos_By_DeviceType(ModularCANLib_DeviceType_Type device_type, ModularCANLib_DeviceInfo_Type ***device_info_ptrs, uint32_t *size);

/**
 * @brief 名前に対応するデバイス情報を取得する
 * * ルックアップテーブルによりO(1)で取得します。
 * * @param[in] name  取得対象の名前（0始まりのenum値、最大MODULARCANLIB_MAX_DEVICES-1）
 * @return ModularCANLib_DeviceInfo_Type* 対応するデバイス情報へのポインタ。未登録の場合はNULL。
 */
ModularCANLib_DeviceInfo_Type* ModularCANLib_GetDeviceInfo_By_Name(uint32_t name);

#endif //SWERVE_CONTROL_BOARD_FIRMWARE_MODULARCANLIB_DEVICEINFO_LOOKUP_H
