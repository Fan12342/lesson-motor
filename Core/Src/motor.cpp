//
// Created by Administrator on 2026/10/3.
//

#include "motor.h"

Motor::Motor(float ratio) : ratio_(ratio) {}

void Motor::canRxMsgCallback(const uint8_t rx_data[8])
{
    last_ecd_ = ecd_angle_;

    uint16_t angle = rx_data[0] << 8 | rx_data[1];
    ecd_angle_ = (float)angle * 360 / 8192;

    int16_t speed = rx_data[2] << 8 | rx_data[3];
    speedRpm_ = (float)(speed / ratio_);

    int16_t current = rx_data[4] << 8 | rx_data[5];
    currentA_ = (float)current * 20 / 16384;

    tempC_ = (rx_data[6] << 8 | rx_data[7]);

    float delta_angle = ecd_angle_ - last_ecd_;
    if (delta_angle <= -180)
        angle_ += 360;
    else if (delta_angle >= 180)
        angle_ -= 360;
    angle_ += delta_angle;
}

float Motor::angle() const
{
    return ecd_angle_;
}

float Motor::speedRpm() const
{
    return speedRpm_;
}

float Motor::currentAmps() const
{
    return currentA_;
}

float Motor::temperatureC() const
{
    return tempC_;
}

void Motor::setTxCurrent(float amperes, uint8_t motor_id)
{
    int16_t data;
    if (amperes > 20)
        data = 16384;
    else if (amperes < -20)
        data = -16384;
    else
        data = (int16_t)(amperes * 16384 / 20);

    if (motor_id <= 4)
    {
        tx_data_[motor_id * 2 - 2] = data >> 8;
        tx_data_[motor_id * 2 - 1] = data & 0xff;
    }else
    {
        // 改0x1ff
    }
}

uint8_t* Motor::getTxData()
{
    return tx_data_;
}
