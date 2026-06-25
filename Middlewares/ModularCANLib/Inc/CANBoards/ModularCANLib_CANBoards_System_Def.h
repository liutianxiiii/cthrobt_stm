/*
 * CAN_System_Def.h
 *
 *  Created on: 9/25, 2021
 *      Author: Emile
 */

#ifndef INC_CAN_SYSTEM_CAN_SYSTEM_DEF_H_
#define INC_CAN_SYSTEM_CAN_SYSTEM_DEF_H_

#include "ModularCANLib_DeviceInfo_Def.h"

uint32_t Make_CAN_ID_from_CAN_Device(const ModularCANLib_DeviceInfo_Type*device_info, uint8_t cmd);  // CAN_IDを生成する

/*
 *  Base ID : 14bit
 *  Data fieldのData : Max 64bit
 *
 * メインからモタドラなどへの送信
 *---------------------------
 * BIT     |内容
 * --------|-----------------
 * [13:11] |Destination NodeType
 * --------|-----------------
 * [10:8]  |Destination NodeID
 * --------|-----------------
 * [7:5]   | DeviceNum
 * --------|-----------------
 * [4:0]   | CommandType
 *---------------------------
 *
 * Destinationのところだけを取ってくるなら,
 * MaskIdHigh = MAKE_MAIN_CANID(0b111, 0b111, 0) << 5; とすればok.
 * 16bitの前14bitがIDに対応する. この14bitをいいかんじに操作してやる.
 *モタドラなどからメインへの送信
 *---------------------------
 * BIT     |内容
 * --------|-----------------
 * [13:11] |Source NodeType
 * --------|-----------------
 * [10:8]  |Source NodeID
 * --------|-----------------
 * [7:5]   | DeviceNum
 * --------|-----------------
 * [4:0]   | CommandType
 *---------------------------
 *
 * フローを書く
 */

#define CMD_AWAKE 0x0

#endif /* INC_CAN_SYSTEM_CAN_SYSTEM_DEF_H_ */
