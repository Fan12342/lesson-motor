//
// Created by Administrator on 2026/10/3.
//

#ifndef LESSON_CAN_CAN_USER_H
#define LESSON_CAN_CAN_USER_H

#include "can.h"

extern CAN_RxHeaderTypeDef rx_header;
extern CAN_TxHeaderTypeDef tx_header;
extern uint32_t can_tx_mailbox;
extern CAN_FilterTypeDef can_filter_config;

#endif //LESSON_CAN_CAN_USER_H
