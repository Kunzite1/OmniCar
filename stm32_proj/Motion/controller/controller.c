/**
  ******************************************************************************
  * @file    Motion/controller/controller.c
  * @brief   运动控制（指令执行 / 轮速闭环调度）
  ******************************************************************************
  */
#include "Motion/controller/controller.h"

#include <stdint.h>

/* 每路电机极性：机械安装标定用，+1 正向 / -1 反向（上板验证后调整） */
static const int8_t s_motor_polarity[MOTOR_NUM] = {
    1,  /* MOTOR_1 */
    1,  /* MOTOR_2 */
    1,  /* MOTOR_3 */
};

/**
  * @brief 按有符号轮速驱动三路电机（开环）
  * @note  符号 → 方向；绝对值 × CONTROLLER_DUTY_PER_UNIT → 占空比，钳位 0~100%
  */
void Controller_SetWheelSpeeds(const float wheel[3])
{
    uint32_t i;

    for (i = 0U; i < MOTOR_NUM; i++)
    {
        float speed;
        MotorDir dir;
        uint32_t percent;

        /* 轮速叠加上该路电机极性（机械安装方向标定） */
        speed = wheel[i] * (float)s_motor_polarity[i];

        /* 转速过小视为停 */
        if ((speed > -0.05f) && (speed < 0.05f))
        {
            BSP_Motor_Stop((MotorId)i);
            continue;
        }

        /* 方向由符号决定，占空比用绝对值 */
        if (speed > 0.0f)
        {
            dir = MOTOR_DIR_FWD;
        }
        else
        {
            dir = MOTOR_DIR_REV;
            speed = -speed;
        }

        percent = (uint32_t)(speed * (float)CONTROLLER_DUTY_PER_UNIT);
        if (percent > 100U)
        {
            percent = 100U;
        }

        BSP_Motor_SetDir((MotorId)i, dir);
        BSP_Motor_SetDuty((MotorId)i, percent);
    }
}
