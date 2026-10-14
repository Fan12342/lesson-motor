//
// Created by Administrator on 2026/10/3.
//
#include "can_user.h"
#include "motor.h"
#include "rm_log.h"

extern bool stop_flag;
extern uint16_t target_angle;

PID_User ppid(13.0f, 0.1f, 15.0f, 0.0f, 100.0f, 0.95f);
PID_User spid(0.7f, 0.0f, 0.0f, 0.0f, 10.0f, 0.03f);
Motor motor1(1, 36.0f, 2, ppid, spid);

constexpr uint32_t kMotor1FeedbackId = 0x201;

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    uint8_t data[8];
    if (hcan != &hcan1) return ;
    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header, data) != HAL_OK) return ;

    if (rx_header.IDE == CAN_ID_STD && rx_header.RTR == CAN_RTR_DATA
        && rx_header.DLC == 8 && rx_header.StdId == kMotor1FeedbackId)
    {
        motor1.canRxMsgCallback(data);

        motor1.handle(target_angle);
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance != TIM6) return;
    if (HAL_CAN_GetTxMailboxesFreeLevel(&hcan1) > 0)
    {
        if (stop_flag)
        {
            motor1.setTxCurrent(0);
        }
        HAL_CAN_AddTxMessage(&hcan1, &tx_header, motor1.getTxData(), &can_tx_mailbox);
    }
}

extern "C" void motor1_setTxCurrent(float amperes)
{
    motor1.setTxCurrent(amperes);
}
