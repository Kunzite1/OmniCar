/**
  ******************************************************************************
  * @file    Middleware/can_protocol/can_protocol.c
  * @brief   与上位机的 CAN 通信协议：组帧与解析（协议表见 can_protocol.h）
  ******************************************************************************
  */
#include "Middleware/can_protocol/can_protocol.h"

#include <string.h>

/**
  * @brief 发送心跳帧 0x101
  */
void CanProto_SendHeartbeat(uint8_t seq)
{
    uint8_t payload[8] = {0};

    payload[0] = seq;
    payload[1] = CANPROTO_FW_VER_MAJOR;
    payload[2] = CANPROTO_FW_VER_MINOR;
    payload[3] = CANPROTO_FW_VER_PATCH;
    /* payload[4..5] = flags（预留 0），payload[6..7] = rfu */

    (void)BSP_CAN_Send(CANPROTO_ID_HEARTBEAT, payload, 8U);
}

/**
  * @brief 对 0x2FF 请求回 echo 应答 0x2FE
  */
void CanProto_SendEchoRsp(const uint8_t *req, uint8_t len)
{
    uint8_t payload[8] = {0};
    uint8_t copy       = (len > 7U) ? 7U : len; /* 首字节留给 seq，回显最多 7 字节 */

    if ((req == NULL) || (len == 0U))
    {
        return;
    }

    payload[0] = req[0]; /* seq = 请求首字节 */
    if (copy > 1U)
    {
        memcpy(&payload[1], &req[1], (size_t)(copy - 1U));
    }

    (void)BSP_CAN_Send(CANPROTO_ID_ECHO_RSP, payload, 8U);
}

/**
  * @brief 按协议 ID 解析收到的帧
  */
bool CanProto_Parse(const BSP_CanFrame *frame, CanProtoMsg *msg)
{
    if ((frame == NULL) || (msg == NULL))
    {
        return false;
    }

    msg->len = (frame->len > 8U) ? 8U : frame->len;
    memcpy(msg->data, frame->data, msg->len);

    switch (frame->id)
    {
        case CANPROTO_ID_HEARTBEAT: msg->type = CANPROTO_MSG_HEARTBEAT; return true;
        case CANPROTO_ID_ECHO_REQ:  msg->type = CANPROTO_MSG_ECHO_REQ;  return true;
        case CANPROTO_ID_ECHO_RSP:  msg->type = CANPROTO_MSG_ECHO_RSP;  return true;
        case CANPROTO_ID_VELOCITY:  msg->type = CANPROTO_MSG_VELOCITY;  return true;
        default:                    msg->type = CANPROTO_MSG_NONE;      return false;
    }
}
