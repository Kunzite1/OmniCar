/**
  ******************************************************************************
  * @file    Motion/controller/controller.h
  * @brief   运动控制（指令执行 / 轮速闭环调度）
  ******************************************************************************
  */
#ifndef MOTION_CONTROLLER_H
#define MOTION_CONTROLLER_H

#include "BSP/motor/motor.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 轮速 → 占空比换算系数：1 单位轮速对应的占空比百分数（开环标定用） */
#define CONTROLLER_DUTY_PER_UNIT 20U

/**
  * @brief 按有符号轮速驱动三路电机（开环）
  * @param wheel[3] 三轮速（有符号），wheel[0..2] 对应 MOTOR_1..3
  * @note  符号 → 方向；绝对值 × 系数 → 占空比（钳位 0~100%）。
  *        每路电机极性在 controller.c 顶部 s_motor_polarity 表标定。
  */
void Controller_SetWheelSpeeds(const float wheel[3]);

#ifdef __cplusplus
}
#endif

#endif /* MOTION_CONTROLLER_H */
