/**
  ******************************************************************************
  * @file    BSP/motor/motor.h
  * @brief   520 编码电机驱动（PWM + 方向）
  ******************************************************************************
  */
#ifndef BSP_MOTOR_H
#define BSP_MOTOR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 电机编号：对应 TIM3 通道与方向 GPIO（见 motor.c 映射表） */
typedef enum {
    MOTOR_1,   /* PWM: TIM3_CH1(PA6)，方向: PE13(M1_IN1)/PE14(M1_IN2) */
    MOTOR_2,   /* PWM: TIM3_CH2(PA7)，方向: PA4(M2_IN1)/PA5(M2_IN2)  */
    MOTOR_3,   /* PWM: TIM3_CH3(PB0)，方向: PD14(M3_IN1)/PD15(M3_IN2) */
    MOTOR_NUM
} MotorId;

/* 电机方向 */
typedef enum {
    MOTOR_DIR_STOP,  /* 停（IN1=IN2=0） */
    MOTOR_DIR_FWD,   /* 正转（IN1=1, IN2=0） */
    MOTOR_DIR_REV    /* 反转（IN1=0, IN2=1） */
} MotorDir;

/**
  * @brief 电机初始化：启动 TIM3 三路 PWM（20 kHz），全部停止
  * @note  需 CubeMX 已配置 TIM3 与方向 GPIO；App_Init 中调用
  */
void BSP_Motor_Init(void);

/**
  * @brief 设置某路电机占空比（调速）
  * @param id      电机编号
  * @param percent 0~100（%），映射到 PWM 计数值 ARR=4199
  */
void BSP_Motor_SetDuty(MotorId id, uint32_t percent);

/**
  * @brief 设置某路电机方向
  * @note  FWD/REV 的正反以驱动芯片 IN1/IN2 逻辑为准；
  *        极性不对时在上板观察后调整接线或此处交换 IN1/IN2
  */
void BSP_Motor_SetDir(MotorId id, MotorDir dir);

/**
  * @brief 停止单路电机（方向复位 + 占空比归零）
  */
void BSP_Motor_Stop(MotorId id);

/**
  * @brief 停止全部电机
  */
void BSP_Motor_StopAll(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_MOTOR_H */
