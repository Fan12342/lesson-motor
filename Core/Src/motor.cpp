//
// Created by Administrator on 2026/10/3.
//

#include "motor.h"

#include <math.h>

#include "rm_log.h"

Motor::Motor(uint8_t motor_id, float ratio, uint8_t control_mode, PID_User ppid, PID_User spid):
    motor_id_(motor_id), ratio_(ratio), control_mode_(control_mode), ppid_(ppid), spid_(spid) {}

void Motor::canRxMsgCallback(const uint8_t rx_data[8])
{
    last_ecd_ = ecd_angle_;

    uint16_t angle = rx_data[0] << 8 | rx_data[1];
    ecd_angle_ = (float)angle;

    int16_t speed = (int16_t)(rx_data[2] << 8 | rx_data[3]);
    speedRpm_ = (float)(speed / ratio_);

    /*
    int16_t current = (int16_t)(rx_data[4] << 8 | rx_data[5]);
    currentA_ = (float)current * 20 / 16384;

    tempC_ = rx_data[6];
    */

    float delta_angle = (ecd_angle_ - last_ecd_);
    if (delta_angle <= -half_kEncoderRange)
        delta_angle += kEncoderRange;
    else if (delta_angle >= half_kEncoderRange)
        delta_angle -= kEncoderRange;
    angle_ += delta_angle * 360 / kEncoderRange / ratio_;
}

float Motor::angle() const
{
    return angle_;
}

float Motor::speedRpm() const
{
    return speedRpm_;
}

void Motor::setTxCurrent(float amperes)
{
    int16_t data;
    if (amperes > 20)
        data = 16384;
    else if (amperes < -20)
        data = -16384;
    else
        data = (int16_t)(amperes * 16384 / 20);

    if (motor_id_ <= 4)
    {
        tx_data_[motor_id_ * 2 - 2] = data >> 8;
        tx_data_[motor_id_ * 2 - 1] = data & 0xff;
    }else
    {
        // 改0x1ff
    }
}

uint8_t* Motor::getTxData()
{
    return tx_data_;
}

void Motor::handle(float arg)
{
    float amperes = 0.0;
    switch (control_mode_)
    {
        case 1:
            {
                float s_ref = ppid_.calc(arg, angle_);
                amperes = spid_.calc(s_ref, speedRpm_);
                break;
            }

        case 2:
            {
                amperes = spid_.calc(0, speedRpm_);
                //amperes = spid_.calc(arg, speedRpm_);
                break;
            }
    }
    amperes += (forward_k) * (float)sin((double)angle_ * 3.1415926f / 180.0f);
    amperes_ = amperes;
    setTxCurrent(amperes);
}
