//
// Created by Administrator on 2026/10/3.
//

#ifndef LESSON_CAN_MOTOR_H
#define LESSON_CAN_MOTOR_H

#include <cstdint>
#include "pid.h"

class Motor
{
public:
    explicit Motor(uint8_t motor_id, float ratio, uint8_t control_mode, PID_User ppid, PID_User spid);

    void canRxMsgCallback(const uint8_t rx_data[8]);
    float amperes_ = 0;

    float angle() const;
    float speedRpm() const;
    float forward_k = 0.43;
    //bool hasFeedback() const;

    void setTxCurrent(float amperes);

    uint8_t* getTxData();

    void handle(float arg);
private:
    const uint8_t motor_id_;
    const float ratio_;
    const uint8_t control_mode_;
    // 1:POSITION_SPEED
    // 2:SPEED

    float ecd_angle_ = 0;
    float speedRpm_ = 0;

    float angle_ = 0;
    float last_ecd_ = 0;
    //bool received_ = false;

    uint8_t tx_data_[8] = {};

    static constexpr uint16_t kEncoderRange = 8192;
    static constexpr uint16_t half_kEncoderRange = kEncoderRange / 2;

    PID_User ppid_;
    PID_User spid_;
};

#endif //LESSON_CAN_MOTOR_H
