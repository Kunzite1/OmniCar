/**
  ******************************************************************************
  * @file    App/cmd_handler/app_cmd_handler.c
  * @brief   上位机指令处理（CAN 帧分发）
  * @note    v0.7.4 链路自检：收到 0x2FF echo 请求回 0x2FE；
  *          速度指令 0x201 等待电机闭环（编码器 + PID）就绪后接入
  ******************************************************************************
  */
#include "App/cmd_handler/app_cmd_handler.h"

#include "BSP/can/can.h"
#include "Middleware/can_protocol/can_protocol.h"
#include "Middleware/log/log.h"
#include "FreeRTOS.h"
#include "task.h"

/**
  * @brief CAN 指令处理任务主体：阻塞收帧 → 解析 → 分发
  */
void App_CmdHandler_Task(void *argument)
{
    BSP_CanFrame frame;
    CanProtoMsg  msg;

    (void)argument;

    LOG_INFO("cmdHandler task started");

    for (;;)
    {
        if (!BSP_CAN_Receive(&frame, portMAX_DELAY))
        {
            continue; /* portMAX_DELAY 下不会超时，保底循环 */
        }

        LOG_INFO("CAN rx id=0x%03X len=%u", frame.id, frame.len);

        if (!CanProto_Parse(&frame, &msg))
        {
            LOG_WARN("unknown CAN id 0x%03X", frame.id);
            continue;
        }

        switch (msg.type)
        {
            case CANPROTO_MSG_ECHO_REQ:
                CanProto_SendEchoRsp(msg.data, msg.len);
                LOG_INFO("echo req -> rsp 0x2FE");
                break;

            default: /* 心跳/速度指令等：链路自检阶段暂不处理 */
                break;
        }
    }
}
