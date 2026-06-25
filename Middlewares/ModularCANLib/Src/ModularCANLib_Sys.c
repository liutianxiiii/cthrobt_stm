//
// Created by tomoya on 2026/04/20.
//
#include "ModularCANLib_Sys.h"
#include "main.h"

#include <string.h>
#include "can.h"

#include "ModularCANLib_Def.h"
#include "ModularCANLib_RTOS.h"

// 変数定義
CAN_RxRingBuffer rx_std_ring[NUM_OF_CANS]; // Standard IDバッファ {CAN1, CAN2, ...}
CAN_RxRingBuffer rx_ext_ring[NUM_OF_CANS]; // Extended IDバッファ {CAN1, CAN2, ...}

CAN_TxRingBuffer tx_std_ring[NUM_OF_CANS]; // Standard IDバッファ {CAN1, CAN2, ...}
CAN_TxRingBuffer tx_ext_ring[NUM_OF_CANS]; // Extended IDバッファ {CAN1, CAN2, ...}

// 使用されている（~有効化されているhcanを記録） CANxが使用されているなら→...[x-1]=hcanx
CAN_HandleTypeDef *modularCANLib_UsedCANHandlers[NUM_OF_CANS] = {NULL};

uint32_t mailbox_start_tick[NUM_OF_CANS][3]; // mailboxウォッチドッグ用タイムスタンプ

// prototype
static HAL_StatusTypeDef ModularCANLib_FilterBank_Init_OneCAN(CAN_HandleTypeDef* hcan);
static void ModularCANLib_TryTransmit_GivenIDType(CAN_HandleTypeDef* hcan, ModularCANLib_CANId_Type can_id_t);
static HAL_StatusTypeDef ModularCANLib_Send_Message(CAN_HandleTypeDef* hcan, uint32_t id, ModularCANLib_CANId_Type id_type, const uint8_t *data, uint8_t size);
static CAN_RxRingBuffer* ModularCANLib_hcan2rx_ring_ptr(const CAN_HandleTypeDef *hcan, ModularCANLib_CANId_Type can_id_t);
static CAN_TxRingBuffer* ModularCANLib_hcan2tx_ring_ptr(const CAN_HandleTypeDef *hcan, ModularCANLib_CANId_Type can_id_t);

// 関数定義
static HAL_StatusTypeDef ModularCANLib_FilterBank_Init_OneCAN(CAN_HandleTypeDef* hcan)
{
    HAL_StatusTypeDef ret = HAL_OK;

    const uint32_t base_bank = hcan->Instance == CAN2 ? 14 : 0;
    CAN_FilterTypeDef sFilterConfig;

    // ====================================================
    // Bank 0: すべてを FIFO0 で受信
    // ====================================================
    sFilterConfig.FilterBank = base_bank;                      // 優先順位が高い（Bank 0）
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;

    // マスクを0にして「すべて通過（Don't care）」にする
    sFilterConfig.FilterIdHigh = 0x0000;
    sFilterConfig.FilterIdLow = 0x0000;
    sFilterConfig.FilterMaskIdHigh = 0x0000;
    sFilterConfig.FilterMaskIdLow = 0x0000;

    sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0; // FIFO0 を指定
    sFilterConfig.FilterActivation = ENABLE;
    sFilterConfig.SlaveStartFilterBank = 14;

    if (HAL_CAN_ConfigFilter(hcan, &sFilterConfig) != HAL_OK) {
        Error_Handler();
        ret |= HAL_ERROR;
    }

    // ====================================================
    // Bank 1: 残りのすべてのメッセージを FIFO1 で受信　（ニート）
    // ====================================================
    sFilterConfig.FilterBank = base_bank + 1;                      // 優先順位が低い（Bank 1）
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;

    // マスクを0にして「すべて通過（Don't care）」にする
    sFilterConfig.FilterIdHigh = 0x0000;
    sFilterConfig.FilterIdLow = 0x0000;
    sFilterConfig.FilterMaskIdHigh = 0x0000;
    sFilterConfig.FilterMaskIdLow = 0x0000;

    sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO1; // FIFO1 を指定
    // Activationなどはそのまま使い回す

    if (HAL_CAN_ConfigFilter(hcan, &sFilterConfig) != HAL_OK) {
        Error_Handler();
        ret |= HAL_ERROR;
    }

    return ret;
}

HAL_StatusTypeDef ModularCANLib_Sys_FilterBank_Init()
{
    HAL_StatusTypeDef ret = HAL_OK;
    for (int i = 0; i < NUM_OF_CANS; i++)
    {
        if (modularCANLib_UsedCANHandlers[i] == NULL)
        {
            MODULARCANLIB_PRINTF_DEBUG("CAN%d is not used -> filter bank init: skipped\n", i+1);
            continue;
        }

        MODULARCANLIB_PRINTF_DEBUG("CAN%d is used -> filter bank init: ", i+1);

        if (ModularCANLib_FilterBank_Init_OneCAN(modularCANLib_UsedCANHandlers[i]) != HAL_OK) {
            Error_Handler();
            ret |= HAL_ERROR;
            MODULARCANLIB_PRINTF_DEBUG("failed\n");
            continue;
        }
        MODULARCANLIB_PRINTF_DEBUG("done\n");
    }

    return ret;
}

HAL_StatusTypeDef ModularCANLib_Sys_ActivateNotification()
{
    HAL_StatusTypeDef ret = HAL_OK;

    // 有効化する割り込みフラグのまとめ
    const uint32_t active_it = CAN_IT_RX_FIFO0_MSG_PENDING |
                               CAN_IT_RX_FIFO1_MSG_PENDING |
                               CAN_IT_TX_MAILBOX_EMPTY |
                                CAN_IT_ERROR_WARNING |
                               CAN_IT_ERROR_PASSIVE |
                               CAN_IT_BUSOFF |
                               CAN_IT_LAST_ERROR_CODE |
                               CAN_IT_ERROR;;

    for (int i = 0; i < NUM_OF_CANS; i++)
    {
        if (modularCANLib_UsedCANHandlers[i] == NULL)
        {
            MODULARCANLIB_PRINTF_DEBUG("CAN%d is not used -> notification init: skipped\n", i+1);
            continue;
        }

        MODULARCANLIB_PRINTF_DEBUG("CAN%d is used -> notification init: ", i+1);

        // 送受信すべての通知を一括で有効化
        if (HAL_CAN_ActivateNotification(modularCANLib_UsedCANHandlers[i], active_it) != HAL_OK)
        {
            Error_Handler();
            ret |= HAL_ERROR;
            MODULARCANLIB_PRINTF_DEBUG("failed\n");
            continue;
        }
        MODULARCANLIB_PRINTF_DEBUG("done\n");
    }

    return ret;
}

HAL_StatusTypeDef ModularCANLib_Sys_MsgPendingCallback(CAN_HandleTypeDef *hcan, const uint32_t rx_fifo)
{
    if (!ModularCANLib_hcanIsUnderControl(hcan)) return HAL_OK;

    CAN_RxHeaderTypeDef rx_header;
    uint8_t rx_data[10];

    while (HAL_CAN_GetRxFifoFillLevel(hcan, rx_fifo) > 0) {
        // FIFOから1つのメッセージを取得
        if (HAL_CAN_GetRxMessage(hcan, rx_fifo, &rx_header, rx_data) == HAL_OK) {
            // リングバッファ取得
            const ModularCANLib_CANId_Type can_id_t = rx_header.IDE == CAN_ID_STD ? ModularCANLib_CAN_ID_TYPE_STD : ModularCANLib_CAN_ID_TYPE_EXT;
            CAN_RxRingBuffer *target_ring = ModularCANLib_hcan2rx_ring_ptr(hcan, can_id_t);

            // ソフトウェアバッファ(ローテーション)へコピー
            const uint32_t next = (target_ring->Head + 1) % CAN_RX_BUF_SIZE;
            if (next != target_ring->Tail) { // バッファオーバーフローのチェック
                target_ring->Buffer[target_ring->Head].Header = rx_header;
                memcpy(target_ring->Buffer[target_ring->Head].Data, rx_data, 8);
                target_ring->Head = next;
            }
            else {
                MODULARCANLIB_PRINTF_DEBUG("Error RX ring buf: FULL\n");
                // return HAL_ERROR;
            }
        }
        else
        {
            MODULARCANLIB_PRINTF_DEBUG("Failed on HAL_CAN_GetRxMessage\n");
            return HAL_ERROR;
        }
    }

    // データ処理フラグを立てる
    osSignalSet(modularCANLibHandle, MODULARCANLIB_TASK_CAN_RX_SIGNAL);
    return HAL_OK;
}

static void ModularCANLib_TryTransmit_GivenIDType(CAN_HandleTypeDef* hcan, const ModularCANLib_CANId_Type can_id_t) {
    const uint8_t can_idx = ModularCANLib_hcan2idx(hcan);
    CAN_TxRingBuffer *target_ring = ModularCANLib_hcan2tx_ring_ptr(hcan, can_id_t);
    uint32_t txMailbox;

    // 割り込みによるバッファ操作の競合を防ぐため割り込みを禁止
    __disable_irq();

    // リングバッファが空ではない ＆ 空きMailboxが1つ以上ある間ループ
    while ((target_ring->Head != target_ring->Tail) &&
           (HAL_CAN_GetTxMailboxesFreeLevel(hcan) > 0)) {

        // テール（読み出し位置）からメッセージを取得
        CAN_TxMsg *msg = &target_ring->Buffer[target_ring->Tail];

        // 送信ヘッダの設定
        if (msg->Header.IDE != CAN_ID_STD && msg->Header.IDE != CAN_ID_EXT) {
            MODULARCANLIB_PRINTF_DEBUG("Tx Header IDE was not set!: %lu\n", msg->Header.IDE);
            // テールを進める
            target_ring->Tail = (target_ring->Tail + 1) % CAN_TX_BUF_SIZE;
            continue;
        }
        msg->Header.RTR = CAN_RTR_DATA;
        msg->Header.TransmitGlobalTime = DISABLE;

        // Mailboxにセットして送信リクエスト
        if (HAL_CAN_AddTxMessage(hcan, &msg->Header, msg->Data, &txMailbox) == HAL_OK) {
            // 送信リクエストに成功
            // 送信時刻記録(WDT用)
            const uint32_t current_tick = HAL_GetTick();
            switch (txMailbox)
            {
            case CAN_TX_MAILBOX0:
                mailbox_start_tick[can_idx][0] = current_tick;
                break;
            case CAN_TX_MAILBOX1:
                mailbox_start_tick[can_idx][1] = current_tick;
                break;
            case CAN_TX_MAILBOX2:
                mailbox_start_tick[can_idx][2] = current_tick;
                break;
            default:
                MODULARCANLIB_PRINTF_DEBUG("Unknown Mailbox: %lu\n", txMailbox);
                break;
            }
            // debug
            // MODULARCANLIB_PRINTF_DEBUG("CAN TX: IDE%u, id%lu->mailbox%u\n",
            //     msg->Header.IDE,
            //     msg->Header.IDE == CAN_ID_STD ? msg->Header.StdId : msg->Header.ExtId,
            //     (uint8_t)txMailbox);
            // テールを進める
            target_ring->Tail = (target_ring->Tail + 1) % CAN_TX_BUF_SIZE;
        } else {
            // 万が一失敗した場合はループを抜ける
            MODULARCANLIB_PRINTF_DEBUG("Failed to AddTxMessage\n");
            break;
        }
    }

    __enable_irq();
}

void ModularCANLib_Sys_TryTransmit(CAN_HandleTypeDef* hcan)
{
    if (!ModularCANLib_hcanIsUnderControl(hcan)) return;

    const CAN_TxRingBuffer *std_ring = ModularCANLib_hcan2tx_ring_ptr(hcan, ModularCANLib_CAN_ID_TYPE_STD);
    if (std_ring->Head != std_ring->Tail)
    {
        ModularCANLib_TryTransmit_GivenIDType(hcan, ModularCANLib_CAN_ID_TYPE_STD);
    }
    const CAN_TxRingBuffer *ext_ring = ModularCANLib_hcan2tx_ring_ptr(hcan, ModularCANLib_CAN_ID_TYPE_EXT);
    if (ext_ring->Head != ext_ring->Tail)
    {
        ModularCANLib_TryTransmit_GivenIDType(hcan, ModularCANLib_CAN_ID_TYPE_EXT);
    }
}

static HAL_StatusTypeDef ModularCANLib_Send_Message(CAN_HandleTypeDef* hcan, uint32_t id, ModularCANLib_CANId_Type id_type, const uint8_t *data, const uint8_t size) {
    CAN_TxRingBuffer *ring = ModularCANLib_hcan2tx_ring_ptr(hcan, id_type);
    const uint16_t next_head = (ring->Head + 1) % CAN_TX_BUF_SIZE;

    __disable_irq();

    // バッファフルチェック
    if (next_head == ring->Tail) {
        __enable_irq();
        MODULARCANLIB_PRINTF_DEBUG("Error TX ring buf: FULL\n");
        return HAL_BUSY; // Buffer Full
    }

    // リングバッファのヘッドにデータを書き込み
    ring->Buffer[ring->Head].Header.IDE = id_type == ModularCANLib_CAN_ID_TYPE_STD ? CAN_ID_STD : CAN_ID_EXT;
    if (id_type == ModularCANLib_CAN_ID_TYPE_STD)
    {
        ring->Buffer[ring->Head].Header.StdId = id;
    }
    else
    {
        ring->Buffer[ring->Head].Header.ExtId = id;
    }
    ring->Buffer[ring->Head].Header.DLC = size;
    memcpy(ring->Buffer[ring->Head].Data, data, 8);

    // ヘッドを進める
    ring->Head = next_head;

    __enable_irq();

    // 空きMailboxがあれば直ちに送信開始（ハードウェアへ移す）
    ModularCANLib_TryTransmit_GivenIDType(hcan, id_type);

    return HAL_OK;
}

static CAN_RxRingBuffer* ModularCANLib_hcan2rx_ring_ptr(const CAN_HandleTypeDef *hcan, const ModularCANLib_CANId_Type can_id_t)
{
    const uint8_t idx = ModularCANLib_hcan2idx(hcan);
    switch (can_id_t)
    {
    case ModularCANLib_CAN_ID_TYPE_STD:
        return &rx_std_ring[idx];
    case ModularCANLib_CAN_ID_TYPE_EXT:
        return &rx_ext_ring[idx];
    default:
        MODULARCANLIB_PRINTF_DEBUG("Unknown can_id_type was given: %u", can_id_t);
        return &rx_std_ring[idx];
    }
}

static CAN_TxRingBuffer* ModularCANLib_hcan2tx_ring_ptr(const CAN_HandleTypeDef *hcan, const ModularCANLib_CANId_Type can_id_t)
{
    const uint8_t idx = ModularCANLib_hcan2idx(hcan);
    switch (can_id_t)
    {
    case ModularCANLib_CAN_ID_TYPE_STD:
        return &tx_std_ring[idx];
    case ModularCANLib_CAN_ID_TYPE_EXT:
        return &tx_ext_ring[idx];
    default:
        MODULARCANLIB_PRINTF_DEBUG("Unknown can_id_type was given: %u", can_id_t);
        return &tx_std_ring[idx];
    }
}

// 公開関数
uint8_t ModularCANLib_hcan2idx(const CAN_HandleTypeDef* hcan)
{
    switch ((uint32_t)hcan->Instance)
    {
#ifdef CAN1
    case (uint32_t)CAN1:
        return 0;
#endif
#ifdef CAN
    case (uint32_t)CAN:
        return 0;
#endif
#ifdef CAN2
    case (uint32_t)CAN2:
        return 1;
#endif
#ifdef CAN3
    case (uint32_t)CAN3:
        return 2;
#endif
    default:
        MODULARCANLIB_PRINTF_DEBUG("Unknown hcan was given: Instance=%lu", (uint32_t)hcan->Instance);
        return 0;
    }
}

bool ModularCANLib_hcanIsUnderControl(const CAN_HandleTypeDef* hcan) {
    switch ((uint32_t)hcan->Instance)
    {
#ifdef CAN1
        case (uint32_t)CAN1:
            return modularCANLib_UsedCANHandlers[0] != NULL;
#endif
#ifdef CAN
        case (uint32_t)CAN:
            return modularCANLib_UsedCANHandlers[0] != NULL;
#endif
#ifdef CAN2
        case (uint32_t)CAN2:
            return modularCANLib_UsedCANHandlers[1] != NULL;
#endif
#ifdef CAN3
        case (uint32_t)CAN3:
            return modularCANLib_UsedCANHandlers[2] != NULL;
#endif
        default:
            MODULARCANLIB_PRINTF_DEBUG("Unknown hcan was given: Instance=%lu", (uint32_t)hcan->Instance);
            return false;
    }
}

HAL_StatusTypeDef ModularCANLib_Sys_ReadFromBuffer(const CAN_HandleTypeDef* hcan, const ModularCANLib_CANId_Type can_id_t, CAN_RxMsg *pDest) {
    // リングバッファ取得
    CAN_RxRingBuffer *ring = ModularCANLib_hcan2rx_ring_ptr(hcan, can_id_t);

    // 1. バッファが空かどうかを確認 (HeadとTailが同じ位置ならデータなし)
    if (ring->Head == ring->Tail) {
        return HAL_ERROR;
    }

    // 2. 最古のデータ(Tailの位置)をDestへコピー
    // 構造体ごとコピーすることで、HeaderとData[8]を一度に転送
    *pDest = ring->Buffer[ring->Tail];

    // 3. Tailポインタをローテーション
    ring->Tail = (ring->Tail + 1) % CAN_RX_BUF_SIZE;

    return HAL_OK;
}

void ModularCANLib_Sys_Check_Tx_Timeout(void) {
    const uint32_t current_tick = HAL_GetTick();
    for (uint8_t can_idx = 0; can_idx < NUM_OF_CANS; can_idx++)
    {
        // Mailbox 0 の監視
        if (HAL_CAN_IsTxMessagePending(&hcan1, CAN_TX_MAILBOX0)) {
            if ((current_tick - mailbox_start_tick[can_idx][0]) > CAN_TX_TIMEOUT_MS) {
                HAL_CAN_AbortTxRequest(&hcan1, CAN_TX_MAILBOX0);
            }
        }

        // Mailbox 1 の監視
        if (HAL_CAN_IsTxMessagePending(&hcan1, CAN_TX_MAILBOX1)) {
            if ((current_tick - mailbox_start_tick[can_idx][1]) > CAN_TX_TIMEOUT_MS) {
                HAL_CAN_AbortTxRequest(&hcan1, CAN_TX_MAILBOX1);
            }
        }

        // Mailbox 2 の監視
        if (HAL_CAN_IsTxMessagePending(&hcan1, CAN_TX_MAILBOX2)) {
            if ((current_tick - mailbox_start_tick[can_idx][2]) > CAN_TX_TIMEOUT_MS) {
                HAL_CAN_AbortTxRequest(&hcan1, CAN_TX_MAILBOX2);
            }
        }
    }
}

HAL_StatusTypeDef ModularCANLib_Sys_SendBytes(CAN_HandleTypeDef* hcan, const uint32_t id, const ModularCANLib_CANId_Type id_type, const uint8_t *bytes, uint32_t size) { // 命令を送信する関数
    const uint32_t quotient = size / 8;
    const uint32_t remainder = size - (8 * quotient);
    HAL_StatusTypeDef ret;

    for (uint32_t i = 0; i < quotient; i++) {
        ret = ModularCANLib_Send_Message(hcan, id, id_type, bytes + i * 8, 8);
        if (ret != HAL_OK) {
            Error_Handler();
            return ret;
        }
    }

    if (remainder != 0) {
        ret = ModularCANLib_Send_Message(hcan, id, id_type, bytes + quotient * 8, remainder);
        if (ret != HAL_OK) {
            Error_Handler();
            return ret;
        }
    }

    return HAL_OK;
}

void ModularCANLib_Sys_DebugPrint() {
    MODULARCANLIB_PRINTF_INFO("STD: RX buf: %lu, TX buf: %lu\n", (rx_std_ring->Head - rx_std_ring->Tail + CAN_RX_BUF_SIZE) % CAN_RX_BUF_SIZE, (tx_std_ring->Head - tx_std_ring->Tail + CAN_TX_BUF_SIZE) % CAN_TX_BUF_SIZE);
}