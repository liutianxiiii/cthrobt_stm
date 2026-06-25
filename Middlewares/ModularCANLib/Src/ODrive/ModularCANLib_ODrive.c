//
// Created by tomoya on 2026/05/09.
//

#include "ODrive/ModularCANLib_ODrive.h"

#include <math.h>
#include "cmsis_os.h"
#include "ModularCANLib_Def.h"
#include "ModularCANLib_DeviceInfo_Def.h"
#include "ModularCANLib_DeviceInfo_Lookup.h"

// 回転方向係数
#define _ROT_COEF(param) (((param)->rotation == ODRIVE_ROT_ACW) ? 1.0f : -1.0f)

// Private Function Prototypes
static void _ODrive_SendBytes(const ModularCANLib_DeviceInfo_Type *device_info,
                              ModularCANLib_ODrive_CmdId cmd_id,
                              const uint8_t *bytes, uint32_t size);
static void _ODrive_SetAbsolutePosition(const ModularCANLib_DeviceInfo_Type *device_info, float pos_user);
static void _ODrive_EncoderOffsetCalibration(ModularCANLib_DeviceInfo_Type *device_info);
static bool _get_switch_state(GPIO_TypeDef *port, uint32_t pin, ModularCANLib_ODrive_SwitchType sw_type);
static void _ODrive_SetFeedbackData(ModularCANLib_DeviceInfo_Type *device_info,
                                    const CAN_RxMsg *rx_msg, uint8_t cmd_id);

// ========== Private Functions ==========

static void _ODrive_SendBytes(const ModularCANLib_DeviceInfo_Type *device_info,
                              const ModularCANLib_ODrive_CmdId cmd_id,
                              const uint8_t *bytes, const uint32_t size)
{
    const uint32_t can_id = (device_info->node_id << 5u) | (uint32_t)cmd_id;
    ModularCANLib_Sys_SendBytes(device_info->hcan, can_id, ModularCANLib_CAN_ID_TYPE_STD, bytes, size);
}

static void _ODrive_SetAbsolutePosition(const ModularCANLib_DeviceInfo_Type *device_info,
                                        const float pos_user)
{
    const ModularCANLib_DeviceParam_ODrive_Type *param = &device_info->device_param.odrive_param;
    float abs_pos_rev = pos_user / param->quant_per_rot * _ROT_COEF(param);
    _ODrive_SendBytes(device_info, ODRIVE_CMD_ID_SET_ABSOLUTE_POSITION,
                      (const uint8_t *)&abs_pos_rev, sizeof(abs_pos_rev));
}

// エンコーダオフセットキャリブレーション（ホールセンサ未使用時にODrive_Initから呼ばれる）
// CAN初期化・接続確認後に呼ぶこと。osDelay使用（Calibration例外）
static void _ODrive_EncoderOffsetCalibration(ModularCANLib_DeviceInfo_Type *device_info)
{
    uint32_t req_state = ODRIVE_AXIS_STATE_ENCODER_OFFSET_CALIBRATION;
    _ODrive_SendBytes(device_info, ODRIVE_CMD_ID_SET_AXIS_STATE,
                      (const uint8_t *)&req_state, sizeof(req_state));

    osDelay(100);

    while (device_info->device_param.odrive_param.feedback.axis_state ==
           ODRIVE_AXIS_STATE_ENCODER_OFFSET_CALIBRATION)
    {
        MODULARCANLIB_PRINTF_DEBUG("[ODrive node%lu] Encoder Offset Calibration...\n", device_info->node_id);
        osDelay(500);
    }
}

static bool _get_switch_state(GPIO_TypeDef *const port, const uint32_t pin,
                              const ModularCANLib_ODrive_SwitchType sw_type)
{
    const GPIO_PinState state = HAL_GPIO_ReadPin(port, (uint16_t)pin);
    return (sw_type == ODRIVE_SWITCH_NO) ? (state != GPIO_PIN_RESET)
                                                       : (state == GPIO_PIN_RESET);
}

static void _ODrive_SetFeedbackData(ModularCANLib_DeviceInfo_Type *device_info,
                                    const CAN_RxMsg *rx_msg, const uint8_t cmd_id)
{
    ModularCANLib_DeviceParam_ODrive_Type *param = &device_info->device_param.odrive_param;
    ModularCANLib_ODrive_FeedbackData_Type    *fb  = &param->feedback;
    ModularCANLib_ODrive_FeedbackDataRaw_Type *raw = &fb->_raw_data;

    raw->_get_counter++;
    if (raw->_get_counter > 128) raw->_get_counter = 128;

    const float rot_coef = _ROT_COEF(param);

    switch (cmd_id) {
        case ODRIVE_CMD_ID_HEARTBEAT:
            // Bytes 0-3: Axis Error, Byte 4: Axis State
            raw->error      = *((const uint32_t *)&rx_msg->Data[0]);
            raw->axis_state = rx_msg->Data[4];
            fb->error       = raw->error;
            fb->axis_state  = raw->axis_state;
            break;

        case ODRIVE_CMD_ID_GET_ERROR:
            raw->error = *((const uint32_t *)&rx_msg->Data[0]);
            fb->error  = raw->error;
            break;

        case ODRIVE_CMD_ID_GET_ENCODER_ESTIMATES:
            raw->pos = *((const float *)&rx_msg->Data[0]);
            raw->vel = *((const float *)&rx_msg->Data[4]);
            fb->position = raw->pos * param->quant_per_rot * rot_coef;
            fb->velocity = raw->vel * param->quant_per_rot * rot_coef;
            break;

        case ODRIVE_CMD_ID_GET_TORQUES:
            raw->torque = *((const float *)&rx_msg->Data[4]);
            fb->torque  = raw->torque;
            break;

        default:
            break;
    }

    fb->get_flag = (raw->_get_counter > 50);
}

// ========== ユーザー向けAPI ==========

void ODrive_Init(ModularCANLib_DeviceInfo_Type *device_info)
{
    ModularCANLib_DeviceParam_ODrive_Type *param = &device_info->device_param.odrive_param;
    param->_enable_flag = false;

    if (!param->is_using_hall_encoder) {
        _ODrive_EncoderOffsetCalibration(device_info);
    }

    _ODrive_SetAbsolutePosition(device_info, param->offset_pos);
    ODrive_ChangeControl(device_info, param->ctrl_type);
}

void ODrive_Calibration(ModularCANLib_DeviceInfo_Type *device_info, const float calib_velocity,
                        const ModularCANLib_ODrive_SwitchType sw_type,
                        GPIO_TypeDef *limit_port, const uint32_t limit_pin)
{
    ModularCANLib_DeviceParam_ODrive_Type *param = &device_info->device_param.odrive_param;
    if (param->use_internal_offset != ODRIVE_USE_OFFSET_POS_CALIB) return;

    const ModularCANLib_ODrive_CtrlType original_ctrl = param->ctrl_type;

    ODrive_ControlDisable(device_info);
    ODrive_ChangeControl(device_info, ODRIVE_CTRL_VEL);
    ODrive_ControlEnable(device_info);

    while (!_get_switch_state(limit_port, limit_pin, sw_type)) {
        MODULARCANLIB_PRINTF_DEBUG("[ODrive node%lu] Calibration...\n", device_info->node_id);
        ODrive_SetTarget(device_info, calib_velocity);
        osDelay(50);
    }
    ODrive_SetTarget(device_info, 0.0f);
    osDelay(100);

    ODrive_ControlDisable(device_info);
    ODrive_ChangeControl(device_info, original_ctrl);
    _ODrive_SetAbsolutePosition(device_info, param->offset_pos);
    // 制御再開はユーザーが ODrive_ControlEnable を呼んで行う
}

void ODrive_ChangeControl(ModularCANLib_DeviceInfo_Type *device_info,
                          const ModularCANLib_ODrive_CtrlType new_ctrl_type)
{
    ModularCANLib_DeviceParam_ODrive_Type *param = &device_info->device_param.odrive_param;
    param->_enable_flag = false;
    param->ctrl_type    = new_ctrl_type;

    uint32_t ctrl_mode_data[2];
    ctrl_mode_data[0] = (uint32_t)new_ctrl_type;
    ctrl_mode_data[1] = (uint32_t)ODRIVE_INPUT_MODE_PASSTHROUGH;
    _ODrive_SendBytes(device_info, ODRIVE_CMD_ID_SET_CONTROLLER_MODE,
                      (const uint8_t *)ctrl_mode_data, sizeof(ctrl_mode_data));

    float pos_gain = param->pos_kp;
    _ODrive_SendBytes(device_info, ODRIVE_CMD_ID_SET_POS_GAIN,
                      (const uint8_t *)&pos_gain, sizeof(pos_gain));

    float vel_gains[2];
    vel_gains[0] = param->vel_kp * param->quant_per_rot; // [N m / (rev/s)]
    vel_gains[1] = param->vel_ki * param->quant_per_rot; // [N m / rev]
    _ODrive_SendBytes(device_info, ODRIVE_CMD_ID_SET_VEL_GAINS,
                      (const uint8_t *)vel_gains, sizeof(vel_gains));

    float limits[2];
    limits[0] = param->velocity_limit / param->quant_per_rot; // [rev/s]
    limits[1] = param->current_limit;                          // [A]
    _ODrive_SendBytes(device_info, ODRIVE_CMD_ID_SET_LIMITS,
                      (const uint8_t *)limits, sizeof(limits));
}

void ODrive_SetTarget(ModularCANLib_DeviceInfo_Type *device_info, const float target_value)
{
    ModularCANLib_DeviceParam_ODrive_Type *param = &device_info->device_param.odrive_param;
    if (!param->_enable_flag) return;
    if (!isfinite(target_value)) return;

    param->_target_value = target_value;

    float req_value;
    switch (param->ctrl_type) {
        case ODRIVE_CTRL_POS:
        case ODRIVE_CTRL_VEL:
            req_value = target_value / param->quant_per_rot;
            break;
        case ODRIVE_CTRL_TORQUE:
            req_value = target_value;
            break;
        default:
            return;
    }
    req_value *= _ROT_COEF(param);

    switch (param->ctrl_type) {
        case ODRIVE_CTRL_POS: {
            float data[2]; // 8 byte
            data[0] = req_value;           // input_pos [rev]
            ((int16_t *)data)[2] = 0;      // input_vel [0.001 rev/s]
            ((int16_t *)data)[3] = 0;      // input_torque [0.001 N m]
            _ODrive_SendBytes(device_info, ODRIVE_CMD_ID_SET_INPUT_POS,
                              (const uint8_t *)data, 8);
            break;
        }
        case ODRIVE_CTRL_VEL: {
            float data[2]; // 8 byte
            data[0] = req_value; // input_vel [rev/s]
            data[1] = 0.0f;      // input_torque_ff [N m]
            _ODrive_SendBytes(device_info, ODRIVE_CMD_ID_SET_INPUT_VEL,
                              (const uint8_t *)data, 8);
            break;
        }
        case ODRIVE_CTRL_TORQUE:
            _ODrive_SendBytes(device_info, ODRIVE_CMD_ID_SET_INPUT_TORQUE,
                              (const uint8_t *)&req_value, 4);
            break;
        default:
            break;
    }
}

void ODrive_ControlEnable(ModularCANLib_DeviceInfo_Type *device_info)
{
    device_info->device_param.odrive_param._enable_flag = true;
    uint32_t req_state = ODRIVE_AXIS_STATE_CLOSED_LOOP_CONTROL;
    _ODrive_SendBytes(device_info, ODRIVE_CMD_ID_SET_AXIS_STATE,
                      (const uint8_t *)&req_state, sizeof(req_state));
}

void ODrive_ControlDisable(ModularCANLib_DeviceInfo_Type *device_info)
{
    device_info->device_param.odrive_param._enable_flag = false;
    uint32_t req_state = ODRIVE_AXIS_STATE_IDLE;
    _ODrive_SendBytes(device_info, ODRIVE_CMD_ID_SET_AXIS_STATE,
                      (const uint8_t *)&req_state, sizeof(req_state));
}

// ========== Modular CANLib Sys向け関数群 ==========

void ModularCANLib_ODrive_DeviceParam_Struct_Init(ModularCANLib_DeviceParam_ODrive_Type *device_param)
{
    device_param->_target_value     = 0.0f;
    device_param->_enable_flag      = false;
    device_param->_is_calibrating   = false;
    device_param->_limit_port       = NULL;
    device_param->_limit_pin        = 0;

    ModularCANLib_ODrive_FeedbackData_Type    *fb  = &device_param->feedback;
    ModularCANLib_ODrive_FeedbackDataRaw_Type *raw = &fb->_raw_data;
    fb->get_flag   = 0;
    fb->error      = 0;
    fb->axis_state = 0;
    fb->position   = 0.0f;
    fb->velocity   = 0.0f;
    fb->torque     = 0.0f;
    raw->_get_counter = 0;
    raw->error        = 0;
    raw->axis_state   = 0;
    raw->pos          = 0.0f;
    raw->vel          = 0.0f;
    raw->torque       = 0.0f;
}

void ModularCANLib_ODrive_Device_Init(ModularCANLib_DeviceInfo_Type *device_info)
{
    // 現在はルックアップテーブルへの登録のみ
    // (ModularCANLib_MakeLookupTable が DeviceType別に処理する)
    (void)device_info;
}

void ModularCANLib_ODrive_AfterDeviceInit(void)
{
    // ODriveはタイマー不要（SetTargetを直接呼び出す方式）
    // ODrive_Init はユーザーが WaitForConnect 後に呼ぶ
}

bool ModularCANLib_ODrive_IsDeviceConnected(const ModularCANLib_DeviceInfo_Type *device_info)
{
    const ModularCANLib_ODrive_FeedbackData_Type *fb = &device_info->device_param.odrive_param.feedback;
    if (!fb->get_flag) return false;
    // INITIALIZINGエラービットが消えていれば起動完了
    return !(fb->error & ((uint32_t)1u << ODRIVE_ERROR_INITIALIZING));
}

bool ModularCANLib_ODrive_AllDevice_Connected(void)
{
    ModularCANLib_DeviceInfo_Type **device_info_ptrs;
    uint32_t size;
    ModularCANLib_GetDeviceInfos_By_DeviceType(ModularCANLib_DeviceType_ODrive_Pro,
                                               &device_info_ptrs, &size);
    for (uint32_t i = 0; i < size; i++) {
        if (device_info_ptrs[i] == NULL) continue;
        if (!ModularCANLib_ODrive_IsDeviceConnected(device_info_ptrs[i])) return false;
    }
    return true;
}

bool ModularCANLib_ODrive_IsODriveID_RX(const ModularCANLib_CANId_Type can_id_type,
                                         const uint32_t can_id)
{
    if (can_id_type != ModularCANLib_CAN_ID_TYPE_STD) return false;

    const uint8_t node_id = (uint8_t)(can_id >> 5u);
    if (node_id > ODRIVE_NODE_ID_MAX) return false;

    const uint8_t cmd_id = (uint8_t)(can_id & 0x1Fu);
    switch (cmd_id) {
        case ODRIVE_CMD_ID_HEARTBEAT:
        case ODRIVE_CMD_ID_GET_ERROR:
        case ODRIVE_CMD_ID_GET_ENCODER_ESTIMATES:
        case ODRIVE_CMD_ID_GET_IQ:
        case ODRIVE_CMD_ID_GET_TEMPERATURE:
        case ODRIVE_CMD_ID_GET_BUS_VOLTAGE_CURRENT:
        case ODRIVE_CMD_ID_GET_TORQUES:
        case ODRIVE_CMD_ID_GET_POWERS:
            return true;
        default:
            return false;
    }
}

void ModularCANLib_ODrive_ProcessRxData(const CAN_HandleTypeDef *hcan, const CAN_RxMsg *rx_msg)
{
    const uint32_t can_id  = rx_msg->Header.StdId;
    const uint8_t  node_id = (uint8_t)(can_id >> 5u);
    const uint8_t  cmd_id  = (uint8_t)(can_id & 0x1Fu);

    ModularCANLib_DeviceInfo_Type **device_info_ptrs;
    uint32_t size;
    ModularCANLib_GetDeviceInfos_By_DeviceType(ModularCANLib_DeviceType_ODrive_Pro,
                                               &device_info_ptrs, &size);
    for (uint32_t i = 0; i < size; i++) {
        if (device_info_ptrs[i] == NULL) continue;
        if (device_info_ptrs[i]->hcan != hcan) continue;
        if (device_info_ptrs[i]->node_id != node_id) continue;

        _ODrive_SetFeedbackData(device_info_ptrs[i], rx_msg, cmd_id);
        return;
    }
}