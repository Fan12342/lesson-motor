//
// Created by Administrator on 2026/10/11.
//

#ifndef LESSON_CAN_PID_H
#define LESSON_CAN_PID_H

class PID_User
{
public:
    PID_User(float kp, float ki, float kd, float i_max, float out_max, float d_filter_k);

    void reset();
    float calc(float ref, float fdb);

    float calc_output;

private:
    float kp_, ki_, kd_;
    float i_max_, out_max_, d_filter_k_;

    float err_sum_ = 0;
    float last_err_ = 0;
};

#endif //LESSON_CAN_PID_H
