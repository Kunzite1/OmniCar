/**
  ******************************************************************************
  * @file    App/main/app_main.h
  * @brief   业务层入口（初始化 / 主循环）
  ******************************************************************************
  */
#ifndef APP_MAIN_H
#define APP_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* 模块对外接口将在此声明 */

/**
  * @brief 业务层初始化：系统时钟和外设初始化完成后调用一次
  */
void App_Init(void);

/**
  * @brief 业务层主循环体：由 FreeRTOS 默认任务周期调用
  */
void App_Loop(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_MAIN_H */
