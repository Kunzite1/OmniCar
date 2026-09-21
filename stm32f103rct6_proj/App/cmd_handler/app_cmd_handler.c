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
    uint32_t     unknown_count = 0U;
    bool         receive_error_reported = false;

    (void)argument;

    LOG_INFO("CAN command task entered");

    for (;;)
    {
        if (!BSP_CAN_Receive(&frame, portMAX_DELAY))
        {
            if (!receive_error_reported)
            {
                LOG_ERROR("CAN receive queue is unavailable");
                receive_error_reported = true;
            }
            vTaskDelay(pdMS_TO_TICKS(1000U));
            continue;
        }
        receive_error_reported = false;

        LOG_DEBUG("CAN RX id=0x%03X len=%u", frame.id, frame.len);

        if (!CanProto_Parse(&frame, &msg))
        {
            unknown_count++;
            if ((unknown_count == 1U) || ((unknown_count % 100U) == 0U))
            {
                LOG_WARN("unknown CAN id=0x%03X count=%lu", frame.id, (unsigned long)unknown_count);
            }
            continue;
        }

        switch (msg.type)
        {
            case CANPROTO_MSG_ECHO_REQ:
                if (CanProto_SendEchoRsp(msg.data, msg.len))
                {
                    LOG_INFO("echo request answered with CAN id=0x2FE");
                }
                else
                {
                    LOG_WARN("echo response could not enter a TX mailbox");
                }
                break;

            default: /* 心跳/速度指令等：链路自检阶段暂不处理 */
                break;
        }
    }
}
