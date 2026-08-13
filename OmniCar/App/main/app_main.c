/**
  ******************************************************************************
  * @file    App/main/app_main.c
  * @brief   业务层入口（初始化 / 主循环）
  ******************************************************************************
  */
#include "App/main/app_main.h"

#include "BSP/led/led.h"
#include "BSP/uart/uart.h"
#include "Middleware/log/log.h"
#include "cmsis_os2.h"

/**
  * @brief 业务层初始化：系统时钟和外设初始化完成后调用一次
  */
void App_Init(void)
{
    BSP_LED_Init();
    BSP_UART_Init();

    LOG_INFO("OmniCar App_Init done, boardLED blinking every 500ms");
    LOG_INFO("你好");
}

/**
  * @brief 业务层主循环体：由 FreeRTOS 默认任务周期调用
  */
void App_Loop(void)
{
    BSP_LED_Toggle();
    osDelay(500U);
}
