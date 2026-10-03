//
// Created by Administrator on 2026/10/3.
//

#ifndef LESSON_CAN_MOTOR_H
#define LESSON_CAN_MOTOR_H

#include <cstdint>

class Motor
{
public:
    explicit Motor(float ratio);

    void canRxMsgCallback(const uint8_t rx_data[8]);

    float angle() const;
    float speedRpm() const;
    float currentAmps() const;
    float temperatureC() const;
    //bool hasFeedback() const;

    void setTxCurrent(float amperes, uint8_t motor_id);

    uint8_t* getTxData();
    float ecd_angle_ = 0;
    float speedRpm_ = 0;
    float currentA_ = 0;
    float tempC_ = 0;
    float angle_ = 0;
private:
    const float ratio_;

    float last_ecd_ = 0;
    bool received_ = false;

    uint8_t tx_data_[8] = {};

    static constexpr uint16_t kEncoderRange = 8192;

};

#endif //LESSON_CAN_MOTOR_H
