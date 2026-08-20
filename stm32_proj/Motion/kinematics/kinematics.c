/**
  ******************************************************************************
  * @file    Motion/kinematics/kinematics.c
  * @brief   三全向轮运动学解算（vx,vy,ω → 三轮速）
  *
  *         逆解公式（轮 i 滚动方向与车体 x 轴夹角 θi）：
  *             wi = -sin(θi)·vx + cos(θi)·vy + R·ω
  *         θ 按 0° / 120° / 240° 布置，R 为底盘半径（轮心到车体中心距离）。
  *         vx>0（前进）时轮 1 不转、轮 2 反、轮 3 正——极性随机械安装而定，
  *         若实际方向不符，上板验证后调整系数符号或电机接线。
  *         单位：vx/vy 米/秒，w 弧度/秒，输出 wheel 米/秒。
  ******************************************************************************
  */
#include "Motion/kinematics/kinematics.h"

void Kinematics_Inverse(float vx, float vy, float w, float wheel[3])
{
    const float R = 0.110f; /* 底盘半径：110 mm（轮心到车体中心） */

    wheel[0] =             0.0f * vx +  1.0f * vy + R * w;
    wheel[1] = -0.8660254f * vx - 0.5f * vy + R * w;
    wheel[2] =  0.8660254f * vx - 0.5f * vy + R * w;
}
