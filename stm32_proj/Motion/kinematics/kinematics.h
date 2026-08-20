/**
  ******************************************************************************
  * @file    Motion/kinematics/kinematics.h
  * @brief   三全向轮运动学解算（vx,vy,ω → 三轮速）
  ******************************************************************************
  */
#ifndef MOTION_KINEMATICS_H
#define MOTION_KINEMATICS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
  * @brief 三轮全向运动学逆解：车体速度 → 三轮速
  * @param vx    平动速度（车体坐标，正 = 前进，单位 m/s）
  * @param vy    平动速度（车体坐标，正 = 向左，单位 m/s）
  * @param w     自转角速度（逆时针为正，单位 rad/s）
  * @param wheel 输出三轮速（有符号，单位 m/s），wheel[0..2] 对应 MOTOR_1..3
  * @note  整车机械参数：底盘半径 R = 110 mm（见 kinematics.c）；轮径 70 mm（轮半径
  *        35 mm，供后续 encoder/PID 做轮速 ↔ 电机转速换算）。θ 按 0°/120°/240° 布置；
  *        实际机械轮子角度不同时需调整 kinematics.c 中的系数。
  */
void Kinematics_Inverse(float vx, float vy, float w, float wheel[3]);

#ifdef __cplusplus
}
#endif

#endif /* MOTION_KINEMATICS_H */
