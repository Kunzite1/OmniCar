/**
  ******************************************************************************
  * @file    App/main/app_main.c
  * @brief   业务层入口（初始化 / 主循环）
  *          v0.7.4：CAN 链路自检——1 Hz 心跳 + echo 应答，电机保持停止
  ******************************************************************************
  */
#include "App/main/app_main.h"

#include "BSP/can/can.h"
#include "BSP/led/led.h"
#include "BSP/motor/motor.h"
#include "BSP/uart/uart.h"
#include "Middleware/can_protocol/can_protocol.h"
#include "Middleware/log/log.h"
#include "cmsis_os2.h"

/* 心跳序号（u8 自然回绕） */
static uint8_t s_heartbeat_seq = 0U;

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

    CanProto_SendHeartbeat(s_heartbeat_seq++);

    osDelay(1000U);
}
