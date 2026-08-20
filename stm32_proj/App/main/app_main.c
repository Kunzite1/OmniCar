/**
  ******************************************************************************
  * @file    App/main/app_main.c
  * @brief   业务层入口（初始化 / 主循环）
  *          v0.7.4：CAN 链路自检——1 Hz 心跳 + echo 应答，电机保持停止
  *          v0.7.7：上电开环直线自检——先停 3 s，向前直行 2 s 后停止
  *          v0.7.8：改为自转自检——先停 3 s，顺时针自转 2 s、逆时针自转 2 s
  ******************************************************************************
  */
#include "App/main/app_main.h"

#include <stdbool.h>
#include <stdint.h>

#include "BSP/can/can.h"
#include "BSP/led/led.h"
#include "BSP/motor/motor.h"
#include "BSP/uart/uart.h"
#include "Middleware/can_protocol/can_protocol.h"
#include "Middleware/log/log.h"
#include "Motion/controller/controller.h"
#include "Motion/kinematics/kinematics.h"
#include "cmsis_os2.h"

/* 心跳序号（u8 自然回绕） */
static uint8_t s_heartbeat_seq = 0U;

/* 上电运动自检是否已执行（每上电只跑一次） */
static bool s_motion_test_done = false;

/**
  * @brief 开环自转自检：先停 3 s 待车放稳，顺时针自转 2 s，逆时针自转 2 s，随后停止
  * @note  w<0 顺时针、w>0 逆时针（运动学约定）；R=0.110 下 w=±9.09 rad/s
  *        → 三轮速 ≈ ±1.0 m/s（约 20% 占空比）。若实际转向与命令相反，交换两个 w 的符号。
  */
static void App_SpinSelfTest(void)
{
    float wheel[3];

    LOG_INFO("spin test: wait 3s");
    osDelay(3000U);

    /* 顺时针自转 2 s：w = -9.09 rad/s（≈ 86 rpm，三轮速 ≈ -1.0 m/s） */
    LOG_INFO("spin test: CW 2s");
    Kinematics_Inverse(0.0f, 0.0f, -9.09f, wheel);
    Controller_SetWheelSpeeds(wheel);
    osDelay(2000U);

    /* 逆时针自转 2 s：w = +9.09 rad/s */
    LOG_INFO("spin test: CCW 2s");
    Kinematics_Inverse(0.0f, 0.0f, 9.09f, wheel);
    Controller_SetWheelSpeeds(wheel);
    osDelay(2000U);

    BSP_Motor_StopAll();
    LOG_INFO("spin test: stopped");
}

/**
  * @brief 业务层初始化：系统时钟和外设初始化完成后调用一次
  */
void App_Init(void)
{
    BSP_LED_Init();
    BSP_UART_Init();
    BSP_CAN_Init();
    BSP_Motor_Init(); /* PWM 起来并保持停止，链路自检阶段不转车 */

    LOG_INFO("OmniCar v0.7.4 CAN link selftest start");
}

/**
  * @brief 业务层主循环体：由 FreeRTOS 默认任务周期调用
  * @note  1 Hz 发心跳帧 0x101（seq 自增）+ LED 翻转；上位机发 0x2FF
  *        测试帧由 canTask 任务回 echo（见 App/cmd_handler）。
  *        上位机侧用 candump can0 看心跳、cansend 发测试帧联调。
  */
void App_Loop(void)
{
    BSP_LED_Toggle();

    /* 上电运动自检：只跑一次（先停 3 s，顺时针/逆时针自转各 2 s） */
    if (!s_motion_test_done)
    {
        s_motion_test_done = true;
        App_SpinSelfTest();
    }

    CanProto_SendHeartbeat(s_heartbeat_seq++);

    osDelay(1000U);
}
