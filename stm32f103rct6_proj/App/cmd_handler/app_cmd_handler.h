/**
  ******************************************************************************
  * @file    App/cmd_handler/app_cmd_handler.h
  * @brief   上位机指令处理（CAN 帧分发）
  ******************************************************************************
  */
#ifndef APP_CMD_HANDLER_H
#define APP_CMD_HANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
  * @brief CAN 指令处理任务：阻塞等待接收帧，按 can_protocol 分发
  * @note  在 freertos.c 的 USER CODE RTOS_THREADS 块内用 osThreadNew 创建；
  *        v0.7.4 链路自检：收到 0x2FF echo 请求回 0x2FE，其余帧打印日志
  */
void App_CmdHandler_Task(void *argument);

#ifdef __cplusplus
}
#endif

#endif /* APP_CMD_HANDLER_H */
