/**
  ******************************************************************************
  * @file    BSP/motor/motor.c
  * @brief   520 编码电机驱动（PWM + 方向）
  * @note    TIM3 20 kHz（ARR=4199），三路 PWM 通道 + 6 路方向 GPIO
  ******************************************************************************
  */
#include "BSP/motor/motor.h"

#include "main.h" /* M1_IN1_Pin / M1_IN1_GPIO_Port 等方向引脚宏 */
#include "tim.h"  /* htim3 */

/* 各电机的 PWM 输出通道 */
static const uint32_t motor_ch[MOTOR_NUM] = {
    TIM_CHANNEL_1,
    TIM_CHANNEL_2,
    TIM_CHANNEL_3,
};

/* 各电机的方向引脚（IN1/IN2，对应驱动芯片 H 桥控制输入） */
typedef struct {
    GPIO_TypeDef *in1_port;
    uint16_t       in1_pin;
    GPIO_TypeDef *in2_port;
    uint16_t       in2_pin;
} MotorPinMap;

static const MotorPinMap motor_pin[MOTOR_NUM] = {
    { M1_IN1_GPIO_Port, M1_IN1_Pin, M1_IN2_GPIO_Port, M1_IN2_Pin },
    { M2_IN1_GPIO_Port, M2_IN1_Pin, M2_IN2_GPIO_Port, M2_IN2_Pin },
    { M3_IN1_GPIO_Port, M3_IN1_Pin, M3_IN2_GPIO_Port, M3_IN2_Pin },
};

void BSP_Motor_Init(void)
{
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);

    BSP_Motor_StopAll();
}

void BSP_Motor_SetDuty(MotorId id, uint32_t percent)
{
    uint32_t pulse;

    if (id >= MOTOR_NUM)
    {
        return;
    }
    if (percent > 100U)
    {
        percent = 100U;
    }
    /* 20 kHz：ARR = 4199，pulse = 4199 * percent / 100 */
    pulse = (uint32_t)((4199U * percent) / 100U);
    __HAL_TIM_SET_COMPARE(&htim3, motor_ch[id], pulse);
}

void BSP_Motor_SetDir(MotorId id, MotorDir dir)
{
    GPIO_PinState in1 = GPIO_PIN_RESET;
    GPIO_PinState in2 = GPIO_PIN_RESET;

    if (id >= MOTOR_NUM)
    {
        return;
    }
    switch (dir)
    {
        case MOTOR_DIR_FWD: in1 = GPIO_PIN_SET;   in2 = GPIO_PIN_RESET; break;
        case MOTOR_DIR_REV: in1 = GPIO_PIN_RESET; in2 = GPIO_PIN_SET;   break;
        case MOTOR_DIR_STOP:
        default:            in1 = GPIO_PIN_RESET; in2 = GPIO_PIN_RESET; break;
    }
    HAL_GPIO_WritePin(motor_pin[id].in1_port, motor_pin[id].in1_pin, in1);
    HAL_GPIO_WritePin(motor_pin[id].in2_port, motor_pin[id].in2_pin, in2);
}

void BSP_Motor_Stop(MotorId id)
{
    BSP_Motor_SetDir(id, MOTOR_DIR_STOP);
    BSP_Motor_SetDuty(id, 0U);
}

void BSP_Motor_StopAll(void)
{
    MotorId i;

    for (i = MOTOR_1; i < MOTOR_NUM; i++)
    {
        BSP_Motor_Stop(i);
    }
}
