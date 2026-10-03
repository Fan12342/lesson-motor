//
// Created by Administrator on 2026/10/3.
//
#include "can_user.h"
#include "motor.h"

Motor motor1(19.2f);
constexpr uint32_t kMotor1FeedbackId = 0x202;

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    uint8_t data[8];
    if (hcan != &hcan1) return ;
    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header, data) != HAL_OK) return ;

    if (rx_header.IDE == CAN_ID_STD && rx_header.RTR == CAN_RTR_DATA
        && rx_header.DLC == 8 && rx_header.StdId == kMotor1FeedbackId)
    {
        motor1.canRxMsgCallback(data);
    }

    motor1.setTxCurrent(1.0f, 2);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance != TIM6) return;
    if (HAL_CAN_GetTxMailboxesFreeLevel(&hcan1) > 0)
    {
        HAL_CAN_AddTxMessage(&hcan1, &tx_header, motor1.getTxData(), &can_tx_mailbox);
    }
}