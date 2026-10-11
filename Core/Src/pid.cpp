//
// Created by Administrator on 2026/10/11.
//

#include "../Inc/pid.h"

PID_User::PID_User(float kp, float ki, float kd, float i_max, float out_max, float d_filter_k):
    kp_(kp), ki_(ki), kd_(kd), i_max_(i_max), out_max_(out_max), d_filter_k_(d_filter_k) {}

void PID_User::reset()
{
    err_sum_ = 0;
    last_err_ = 0;
}

float PID_User::calc(float ref, float fdb)
{
    float error = ref - fdb;
    err_sum_ += error;

    float pout = kp_ * error;
    float iout = ki_ * err_sum_;
    iout = (iout < i_max_) ? iout : i_max_;
    iout = (iout > -i_max_) ? iout : -i_max_;
/*
    float dout = kd_ * (d_error - last_err_);

    last_err_ = d_error;
*/
    // 对 err 滤波
    float d_error = (1 - d_filter_k_) * last_err_ + d_filter_k_ * error;

    // 对 dout 滤波
    static float dout = 0;
    dout = (1 - d_filter_k_) * dout + d_filter_k_ * kd_ * (d_error - last_err_);

    last_err_ = d_error;

    float out = pout + iout + dout;
    out = (out < out_max_) ? out : out_max_;
    out = (out > -out_max_) ? out : -out_max_;

    calc_output = out;

    return out;
}