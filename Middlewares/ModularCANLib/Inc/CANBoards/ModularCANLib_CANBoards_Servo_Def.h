/*
 * ServoDriver_CAN_Def.h
 *
 *  Created on: Jan 11, 2021
 *      Author: yuta2
 */

#ifndef INC_CAN_SERVO_DEF_H_
#define INC_CAN_SERVO_DEF_H_
#include <stdint.h>


typedef struct {
    uint8_t device_num;
    float pulse_width_min;
    float pulse_width_max;
    float pwm_frequency;
    float angle_range;
    float angle_offset;
    bool _get_flag;
}ModularCANLib_DeviceParam_CANBoards_Servo_Type;  // mainからservoにCANで送信するパラメータを持つ構造体


typedef enum {
    SERVO_CMD_AWAKE,
    SERVO_CMD_INIT1,
    SERVO_CMD_INIT2,
    SERVO_CMD_INIT3,
    SERVO_CMD_SET_TARGET,
}ModularCANLib_CANServo_CMD; // CANで送受信するコマンド


#endif /* INC_CAN_SERVO_DEF_H_ */
