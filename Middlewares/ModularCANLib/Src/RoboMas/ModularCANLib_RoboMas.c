//
// Created by tomoya on 2026/04/21.
//

#include "RoboMas/ModularCANLib_RoboMas.h"

#include <math.h>

#include "cmsis_os.h"
#include "ModularCANLib_Def.h"
#include "ModularCANLib_DeviceInfo_Def.h"
#include "ModularCANLib_DeviceInfo_Lookup.h"

// 変数
static ModularCANLib_DeviceInfo_Type *table_CanIdx_To_DeviceInfoPtrs[NUM_OF_CANS][MODULARCANLIB_MAX_NUM_OF_ONE_DEVICE] = {NULL};
static int tableSize_CanIdx_To_DeviceInfoPtrs[NUM_OF_CANS] = {0};

// freeRTOS
osTimerId modularCANLib_RobomasTimerHandle;
osStaticTimerDef_t modularCANLib_RobomasTimerControlBlock;

// Private Function Prototypes --------------------------------
static bool get_switch_state(GPIO_TypeDef* limit_port, uint16_t limit_pin, ModularCANLib_DeviceParam_RoboMas_Switch_Type sw_type);
static float clip_f_abs(float var, float abs_ref);
static int16_t c610_current_f2int(float current);
static int16_t c620_current_f2int(float current);

// freeRTOS
static void ModularCANLib_RobomasTimerCallback(void const * argument);

// Functions --------------------------------
static bool get_switch_state(GPIO_TypeDef* limit_port, const uint16_t limit_pin, const ModularCANLib_DeviceParam_RoboMas_Switch_Type sw_type){
    if(sw_type == ROBOMAS_SWITCH_NO){
        return HAL_GPIO_ReadPin(limit_port, limit_pin);
    }else{
        return !HAL_GPIO_ReadPin(limit_port, limit_pin);
    }
}

static float clip_f_abs(const float var, float abs_ref) {
    abs_ref = fabsf(abs_ref);
    return fmaxf(fminf(var, abs_ref), -abs_ref);
}

static int16_t c610_current_f2int(const float current) {
    return (int16_t) (current * 10000.0f / 10.0f);
}

static int16_t c620_current_f2int(const float current) {
    return (int16_t) (current * 16384.0f / 20.0f);
}

/* robomasTimerCallback function */
static void ModularCANLib_RobomasTimerCallback(void const * argument)
{
    /* USER CODE BEGIN robomasTimerCallback */
    ModularCANLib_RoboMas_SendRequest(1000.0f); // 1 kHzで制御値を計算
    /* USER CODE END robomasTimerCallback */
}

static void change_internal_offset_for_calib(ModularCANLib_DeviceInfo_Type *device_info){
    My_ModularCANLib_RoboMas_FeedbackData_Raw_Type *feedback_raw = &device_info->device_param.robomas_param.feedback._raw_data;
    feedback_raw->_internal_offset_pos = feedback_raw->pos;  // 現在の位置をoffsetに設定
    feedback_raw->_rot_num = 0;  // 回転数をリセット
}

void RoboMas_FinishCalib(ModularCANLib_DeviceInfo_Type *device_info)
{
    ModularCANLib_DeviceParam_RoboMas_Type *robomas_param = &device_info->device_param.robomas_param;
    robomas_param->_is_calibrating = false;

    change_internal_offset_for_calib(device_info);

    RoboMas_ControlDisable(device_info);
    RoboMas_ChangeControl(device_info, robomas_param->_ctrl_type_before_calib);
    if(robomas_param->_ctrl_type_before_calib == ROBOMAS_CTRL_POS){
        RoboMas_SetTarget(device_info, robomas_param->offset_pos);
    }else{
        RoboMas_SetTarget(device_info, 0.0f);
    }
    RoboMas_ControlEnable(device_info);
}

static void ModularCANLib_RoboMas_SendRequest_OneCAN(const float update_freq_hz, CAN_HandleTypeDef* hcan)
{
    const uint8_t can_idx = ModularCANLib_hcan2idx(hcan);

    uint8_t data1[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    uint8_t data2[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    bool flag_1 = false, flag_2 = false;
    int16_t request_value = 0;

    for (int i = 0; i < tableSize_CanIdx_To_DeviceInfoPtrs[can_idx]; i++) {
        ModularCANLib_DeviceInfo_Type *device_info = table_CanIdx_To_DeviceInfoPtrs[can_idx][i];
        ModularCANLib_DeviceParam_RoboMas_Type *robomas_param = &device_info->device_param.robomas_param;
        if (!robomas_param->_enable_flag)continue; // 制御中でない

        // キャリブレーション終了判定
        if (robomas_param->_is_calibrating) {
            if (get_switch_state(robomas_param->_limit_port, robomas_param->_limit_pin, robomas_param->_sw_type)) {
                // Calibration終了
                RoboMas_FinishCalib(device_info);
            }
        }

        // 制御値計算
        float target_current = 0.0f; // A
        switch (robomas_param->ctrl_type)
        {
        case ROBOMAS_CTRL_CURRENT:
            target_current = robomas_param->_target_value;
            break;
        case ROBOMAS_CTRL_VEL:
            target_current = ModularCANLib_Pid_Ctrl(&(robomas_param->pid_vel), robomas_param->_target_value - robomas_param->feedback.velocity, robomas_param->current_limit == ROBOMAS_LIMIT_ENABLE, robomas_param->current_limit_size, update_freq_hz);
            break;
        case ROBOMAS_CTRL_POS:
            {
                const float t_vel = ModularCANLib_Pid_Ctrl(&(robomas_param->pid_pos), robomas_param->_target_value - robomas_param->feedback.position, robomas_param->velocity_limit == ROBOMAS_LIMIT_ENABLE, robomas_param->velocity_limit_size, update_freq_hz);
                target_current = ModularCANLib_Pid_Ctrl(&(robomas_param->pid_vel), t_vel - robomas_param->feedback.velocity, robomas_param->current_limit == ROBOMAS_LIMIT_ENABLE, robomas_param->current_limit_size, update_freq_hz);
            }
            break;
        default:
            MODULARCANLIB_PRINTF_DEBUG("Unknown RoboMas Control Type\n");
            break;
        }

        // 目標値の計算
        switch (device_info->device_type) {
            case ModularCANLib_DeviceType_RoboMas_C610:
                robomas_param->_req_value = clip_f_abs(target_current, 10.0f);
                request_value = c610_current_f2int( robomas_param->_req_value);
                break;
            case ModularCANLib_DeviceType_RoboMas_C620:
                robomas_param->_req_value = clip_f_abs(target_current, 20.0f);
                request_value = c620_current_f2int( robomas_param->_req_value);
                break;
            default:
                request_value = 2; // なにこれ？
                break;
        }

        if(robomas_param->rotation == ROBOMAS_ROT_CW){
            request_value *= -1;
        }

        // 各モーターの目標値の設定
        if (device_info->node_id < 5) {
            flag_1 = true;
            for (uint8_t j = 0; j < 2; j++) {
                data1[(device_info->node_id - 1) * 2 + j] = (request_value >> ((!j) * 8)) & 0b11111111;
            }
        } else if (device_info->node_id >= 5) {
            flag_2 = true;
            for (uint8_t j = 0; j < 2; j++) {
                data2[(device_info->node_id - 5) * 2 + j] = (request_value >> ((!j) * 8)) & 0b11111111;
            }
        }
    }

    // CAN送信
    if (flag_1)ModularCANLib_Sys_SendBytes(hcan, 0x200, ModularCANLib_CAN_ID_TYPE_STD, data1, sizeof(data1));
    if (flag_2)ModularCANLib_Sys_SendBytes(hcan, 0x1FF, ModularCANLib_CAN_ID_TYPE_STD, data2, sizeof(data2));
}

void ModularCANLib_RoboMas_SendRequest(const float update_freq_hz) {
    for (int can_idx = 0; can_idx < NUM_OF_CANS; can_idx++)
    {
        CAN_HandleTypeDef *hcan = modularCANLib_UsedCANHandlers[can_idx];
        if (hcan == NULL) continue;

        // can_idxについて制御
        ModularCANLib_RoboMas_SendRequest_OneCAN(update_freq_hz, hcan);
    }
}

// fb更新用内部関数
static void ModularCANLib_RoboMas_SetFbData(ModularCANLib_DeviceInfo_Type* device_info, const CAN_RxMsg* rx_msg) {
    ModularCANLib_DeviceParam_RoboMas_Type *robomas_param = &device_info->device_param.robomas_param;
    My_ModularCANLib_RoboMas_FeedbackData_Raw_Type *fb_data_raw = &robomas_param->feedback._raw_data;
    fb_data_raw->_get_counter++;
    if (fb_data_raw->_get_counter > 128) {
        fb_data_raw->_get_counter = 128;  // overflow対策
    }

    if (fb_data_raw->_get_counter < 50) {  // Encoderの初期位置を取得
        fb_data_raw->_internal_offset_pos = (uint16_t) (rx_msg->Data[0] << 8 | rx_msg->Data[1]);
        fb_data_raw->pos_pre = (uint16_t) (rx_msg->Data[0] << 8 | rx_msg->Data[1]);
        fb_data_raw->pos = (uint16_t) (rx_msg->Data[0] << 8 | rx_msg->Data[1]);
        return;
    }

    // dataの設定
    fb_data_raw->pos_pre = fb_data_raw->pos;
    fb_data_raw->pos = (uint16_t) (rx_msg->Data[0] << 8 | rx_msg->Data[1]);
    fb_data_raw->vel = (int16_t) (rx_msg->Data[2] << 8 | rx_msg->Data[3]);
    fb_data_raw->cur = (int16_t) (rx_msg->Data[4] << 8 | rx_msg->Data[5]);

    // 回転数の計算
    const int32_t diff_pos = (int32_t) (fb_data_raw->pos) - (int32_t) (fb_data_raw->pos_pre);
    if (diff_pos > 4096) {
        if (fb_data_raw->_rot_num != -(INT64_MAX / 10)) {
            fb_data_raw->_rot_num -= 1;
        }  // overflow対策
    } else if (diff_pos < -4096) {
        if (fb_data_raw->_rot_num != (INT64_MAX / 10)) {
            fb_data_raw->_rot_num += 1;
        }  // overflow対策
    }

    ModularCANLib_RoboMas_FeedbackData_Type *fb_data = &robomas_param->feedback;
    float quant_per_rot = robomas_param->quant_per_rot;
    if(robomas_param->rotation == ROBOMAS_ROT_CW){
        quant_per_rot *= -1.0f;
    }
    const int32_t offset_pos = (int32_t) (fb_data_raw->pos) - (int32_t) (fb_data_raw->_internal_offset_pos);
    if (robomas_param->use_internal_offset != ROBOMAS_USE_OFFSET_POS_DISABLE) {
        fb_data->position = (((float)offset_pos) / 8192.0f + (float) (fb_data_raw->_rot_num)) * quant_per_rot + robomas_param->offset_pos;
    } else {
        fb_data->position = (((float)fb_data_raw->pos) / 8192.0f + (float) (fb_data_raw->_rot_num)) * quant_per_rot + robomas_param->offset_pos;
    }

    fb_data->velocity = ((float) (fb_data_raw->vel)) / 60.0f * quant_per_rot;
    switch (device_info->device_type) {
        case ModularCANLib_DeviceType_RoboMas_C610:
            fb_data->current = ((float) (fb_data_raw->cur * 10)) / 10000.0f;
            break;
        case ModularCANLib_DeviceType_RoboMas_C620:
            fb_data->current = ((float) (fb_data_raw->cur * 20)) / 16384.0f;
            break;
        default:
            fb_data->current = 0.0f;
            break;
    }
    fb_data->get_flag = (fb_data_raw->_get_counter > 50);
}

void RoboMas_Calibration(ModularCANLib_DeviceInfo_Type *device_info, const float calib_vel, const ModularCANLib_DeviceParam_RoboMas_Switch_Type sw_type, GPIO_TypeDef* limit_port, const uint16_t limit_pin){
    if(device_info->device_param.robomas_param.use_internal_offset != ROBOMAS_USE_OFFSET_POS_CALIB) return;

    device_info->device_param.robomas_param._sw_type = sw_type;
    device_info->device_param.robomas_param._limit_port = limit_port;
    device_info->device_param.robomas_param._limit_pin = limit_pin;
    device_info->device_param.robomas_param._ctrl_type_before_calib = device_info->device_param.robomas_param.ctrl_type;

    RoboMas_ControlDisable(device_info);
    RoboMas_ChangeControl(device_info, ROBOMAS_CTRL_VEL);
    RoboMas_SetTarget(device_info, calib_vel);
    RoboMas_ControlEnable(device_info);

    device_info->device_param.robomas_param._is_calibrating = true;
}

void RoboMas_ChangeControl(ModularCANLib_DeviceInfo_Type *device_info, const ModularCANLib_DeviceParam_RoboMas_Ctrl_Type new_ctrl_type) {
    ModularCANLib_DeviceParam_RoboMas_Type *device_param = &device_info->device_param.robomas_param;
    device_param->offset_pos = 0.0f;
    device_param->_target_value = 0.0f;
    device_param->_enable_flag = false;
    device_param->_is_calibrating = false;
    ModularCANLib_Pid_Init(&device_param->pid_pos);
    ModularCANLib_Pid_Init(&device_param->pid_vel);
    device_info->device_param.robomas_param.ctrl_type = new_ctrl_type;
}

void RoboMas_SetTarget(ModularCANLib_DeviceInfo_Type *device_info, const float target_value) {
    if(!isfinite(target_value)) return;
    if(device_info->device_param.robomas_param._is_calibrating) return;

    device_info->device_param.robomas_param._target_value = target_value;
}

void RoboMas_MoveWithRamp(ModularCANLib_DeviceInfo_Type *const device_info, const float target_pos, const float max_speed, const float acc, const float current_ff_gain)
{
    ModularCANLib_DeviceParam_RoboMas_Type *const param = &device_info->device_param.robomas_param;

    if (param->ctrl_type != ROBOMAS_CTRL_POS)
    {
        MODULARCANLIB_PRINTF_DEBUG("[WARN] RoboMas_MoveWithRamp: ctrl_type is not ROBOMAS_CTRL_POS\n");
        return;
    }

    if (!param->_enable_flag) return;
    if (param->_is_calibrating) return;

    const float start_pos = param->feedback.position;
    const float delta_pos = target_pos - start_pos;

    if (fabsf(delta_pos) < 1e-5f)
    {
        param->_target_value = target_pos;
        param->pid_pos.kff = 0.0f;
        param->pid_vel.kff = 0.0f;
        return;
    }

    // 台形 or 三角形の判定
    const bool is_triangle_curve = (max_speed * max_speed / acc) >= fabsf(delta_pos);

    float t1, t2, t3;
    if (is_triangle_curve)
    {
        t1 = t2 = sqrtf(fabsf(delta_pos) / acc);
        t3 = 2.0f * t1;
    }
    else
    {
        t1 = max_speed / acc;
        t2 = t1 + (fabsf(delta_pos) - (max_speed * max_speed / acc)) / max_speed;
        t3 = t1 + t2;
    }

    const float real_acc = acc * (delta_pos > 0 ? 1.0f : -1.0f);
    const float pos_at_t1 = 0.5f * real_acc * t1 * t1;
    const float pos_at_t2 = pos_at_t1 + real_acc * t1 * (t2 - t1);

    const uint32_t st = HAL_GetTick();
    uint32_t nt = HAL_GetTick();
    float dt = (float)(nt - st) * 0.001f;

    // 加速フェーズ
    while (dt < t1)
    {
        param->_target_value = start_pos + 0.5f * real_acc * dt * dt;
        param->pid_pos.kff = real_acc * dt;
        param->pid_vel.kff = current_ff_gain * real_acc;
        osDelay(1);
        nt = HAL_GetTick();
        dt = (float)(nt - st) * 0.001f;
    }

    // 定速フェーズ (三角形プロファイルの場合は t1 == t2 のため即スキップ)
    while (dt < t2)
    {
        param->_target_value = start_pos + pos_at_t1 + real_acc * t1 * (dt - t1);
        param->pid_pos.kff = real_acc * t1;
        param->pid_vel.kff = 0.0f;
        osDelay(1);
        nt = HAL_GetTick();
        dt = (float)(nt - st) * 0.001f;
    }

    // 減速フェーズ
    while (dt < t3)
    {
        const float dt_diff = dt - t2;
        param->_target_value = start_pos + pos_at_t2 + (real_acc * t1 * dt_diff) - (0.5f * real_acc * dt_diff * dt_diff);
        param->pid_pos.kff = real_acc * (t1 - dt_diff);
        param->pid_vel.kff = current_ff_gain * (-real_acc);
        osDelay(1);
        nt = HAL_GetTick();
        dt = (float)(nt - st) * 0.001f;
    }

    // 後処理: 最終目標位置を確定し FF項をゼロに戻す
    param->_target_value = target_pos;
    param->pid_pos.kff = 0.0f;
    param->pid_vel.kff = 0.0f;
}

void RoboMas_ControlEnable(ModularCANLib_DeviceInfo_Type *device_info) {
    device_info->device_param.robomas_param._enable_flag = true;
}

void RoboMas_ControlDisable(ModularCANLib_DeviceInfo_Type *device_info) {
    device_info->device_param.robomas_param._enable_flag = false;
}

bool RoboMas_IsCalibrationEnded(const ModularCANLib_DeviceInfo_Type *device_info){
    return !(device_info->device_param.robomas_param._is_calibrating);
}

void RoboMas_Wait_For_Calib(const ModularCANLib_DeviceInfo_Type *device_info){
    while(device_info->device_param.robomas_param._is_calibrating) osDelay(5);
}

// =======================
// Modular CANLib提供関数たち
// =======================

void ModularCANLib_RoboMas_DeviceParam_Struct_Init(ModularCANLib_DeviceParam_RoboMas_Type *device_param) {
    device_param->_target_value = 0.0f;
    device_param->_enable_flag = false;
    device_param->_is_calibrating = false;
    ModularCANLib_Pid_Init(&device_param->pid_pos);
    ModularCANLib_Pid_Init(&device_param->pid_vel);
    // feedback初期化
    device_param->feedback._raw_data.pos = 0;
    device_param->feedback._raw_data.pos_pre = 0;
    device_param->feedback._raw_data._rot_num = 0;
    device_param->feedback._raw_data.vel = 0;
    device_param->feedback._raw_data.cur = 0;
    device_param->feedback._raw_data._get_counter = 0;
    device_param->feedback._raw_data._internal_offset_pos = 0;
}

void ModularCANLib_RoboMas_Device_Init(ModularCANLib_DeviceInfo_Type *device_info) {
    /* ===================================
     *         TODO Paramチェック
     * ===================================*/
    // Lookupテーブルに登録
    const uint8_t can_idx = ModularCANLib_hcan2idx(device_info->hcan);
    table_CanIdx_To_DeviceInfoPtrs[can_idx][tableSize_CanIdx_To_DeviceInfoPtrs[can_idx]] = device_info;
    tableSize_CanIdx_To_DeviceInfoPtrs[can_idx]++;
}

void ModularCANLib_RoboMas_AfterDeviceInit() {
    bool has_robomas = false;
    for (int idx = 0; idx < NUM_OF_CANS; idx++) {
        has_robomas = has_robomas || tableSize_CanIdx_To_DeviceInfoPtrs[idx] > 0;
    }

    if (!has_robomas) return;

    /* definition and creation of robomasTimer */
    osTimerStaticDef(robomasTimer, ModularCANLib_RobomasTimerCallback, &modularCANLib_RobomasTimerControlBlock);
    modularCANLib_RobomasTimerHandle = osTimerCreate(osTimer(robomasTimer), osTimerPeriodic, NULL);

    MODULARCANLIB_PRINTF_DEBUG("robomasTimer osTimerStart\n");
    osTimerStart(modularCANLib_RobomasTimerHandle, 1); // 1 kHz
}

bool ModularCANLib_RoboMas_IsDeviceConnected(const ModularCANLib_DeviceInfo_Type *dev_info)
{
    return dev_info->device_param.robomas_param.feedback.get_flag;
}

bool ModularCANLib_RoboMas_AllDevice_Connected()
{
    ModularCANLib_DeviceInfo_Type **devinfo_ptrs;
    uint32_t size;
    // C610
    ModularCANLib_GetDeviceInfos_By_DeviceType(ModularCANLib_DeviceType_RoboMas_C610, &devinfo_ptrs, &size);
    for (uint32_t i = 0; i < size; i++)
    {
        if (devinfo_ptrs[i] == NULL) continue;
        if (!ModularCANLib_RoboMas_IsDeviceConnected(devinfo_ptrs[i]))
        {
            // 未接続
            return false;
        }
    }
    // C620
    ModularCANLib_GetDeviceInfos_By_DeviceType(ModularCANLib_DeviceType_RoboMas_C620, &devinfo_ptrs, &size);
    for (uint32_t i = 0; i < size; i++)
    {
        if (devinfo_ptrs[i] == NULL) continue;
        if (!ModularCANLib_RoboMas_IsDeviceConnected(devinfo_ptrs[i]))
        {
            // 未接続
            return false;
        }
    }
    return true; // すべて接続済み
}

bool ModularCANLib_RoboMas_IsRoboMasID_RX(const ModularCANLib_CANId_Type can_id_type, const uint32_t can_id)
{
    if (can_id_type != ModularCANLib_CAN_ID_TYPE_STD) return false;

    return 0x201u <= can_id && can_id <= 0x208u;
}

void ModularCANLib_RoboMas_ProcessRxData(const CAN_HandleTypeDef* hcan, const CAN_RxMsg* rx_msg)
{
    const uint32_t robomas_id = rx_msg->Header.StdId - 0x200;
    if (robomas_id < 1 || robomas_id > 8)
    {
        MODULARCANLIB_PRINTF_DEBUG("Calced robomas id is out of range: %lu\n", robomas_id);
        return;
    }

    // DeviceInfo C610, C620を検索して処理
    ModularCANLib_DeviceInfo_Type **device_info_ptrs;
    uint32_t size;
    // idx0: C610, idx1: C620
    for (int idx = 0; idx < 2; ++idx)
    {
        const ModularCANLib_DeviceType_Type device_type = idx == 0 ? ModularCANLib_DeviceType_RoboMas_C610 : ModularCANLib_DeviceType_RoboMas_C620;
        ModularCANLib_GetDeviceInfos_By_DeviceType(device_type, &device_info_ptrs, &size);
        for (uint32_t i = 0; i < size; i++)
        {
            if (device_info_ptrs[i]->hcan != hcan) continue; // このCANではない
            if (device_info_ptrs[i]->node_id != robomas_id) continue; // ID不一致

            // fb更新
            ModularCANLib_RoboMas_SetFbData(device_info_ptrs[i], rx_msg);

            return; // 探索終了
        }
    }
}