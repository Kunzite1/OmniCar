/**
  ******************************************************************************
  * @file    Middleware/can_protocol/can_protocol.h
  * @brief   与上位机（KICKPI K1 Mini / RK3568）的 CAN 通信协议（v0.1）
  *
  * 协议约定：CAN 2.0 标准数据帧，500 kbps，数据段小端。
  * ID 段划分：0x1xx = STM32 → 上位机（状态类），
  *            0x2xx = 上位机 → STM32（指令类）。
  *
  * | ID    | 方向          | 周期/触发 | DLC | 载荷（小端）                              | 说明                 |
  * |-------|---------------|-----------|-----|-------------------------------------------|----------------------|
  * | 0x101 | STM32→上位机  | 1 Hz      | 8   | u8 seq, u8 ver[3], u16 flags, u16 rfu     | 心跳/状态            |
  * | 0x2FF | 上位机→STM32  | 按需      | ≤8  | 任意（首字节作为 echo 的 seq）             | 链路测试 echo 请求   |
  * | 0x2FE | STM32→上位机  | 请求触发  | 8   | u8 seq, u8 rfu, 请求载荷原样回显           | 链路测试 echo 应答   |
  * | 0x201 | 上位机→STM32  | （预留）  | 8   | i16 vx, i16 vy, i16 ω（量纲待定）          | 速度指令（未实现）   |
  ******************************************************************************
  */
#ifndef MID_CAN_PROTOCOL_H
#define MID_CAN_PROTOCOL_H

#include <stdbool.h>
#include <stdint.h>

#include "BSP/can/can.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 报文 ID（见文件头协议表） */
#define CANPROTO_ID_HEARTBEAT 0x101U /* 心跳/状态，STM32→上位机 */
#define CANPROTO_ID_ECHO_REQ  0x2FFU /* 链路测试请求，上位机→STM32 */
#define CANPROTO_ID_ECHO_RSP  0x2FEU /* 链路测试应答，STM32→上位机 */
#define CANPROTO_ID_VELOCITY  0x201U /* 速度指令（预留），上位机→STM32 */

/* 固件版本（心跳帧上报，与仓库版本号保持一致） */
#define CANPROTO_FW_VER_MAJOR 0U
#define CANPROTO_FW_VER_MINOR 7U
#define CANPROTO_FW_VER_PATCH 4U

/* 解析后的报文类型 */
typedef enum {
    CANPROTO_MSG_NONE,     /* 未知 ID */
    CANPROTO_MSG_HEARTBEAT,
    CANPROTO_MSG_ECHO_REQ,
    CANPROTO_MSG_ECHO_RSP,
    CANPROTO_MSG_VELOCITY, /* 预留：本轮不处理 */
} CanProtoMsgType;

/* 解析结果：类型 + 原始载荷（多字节字段由调用方按小端解释） */
typedef struct {
    CanProtoMsgType type;
    uint8_t         len;
    uint8_t         data[8];
} CanProtoMsg;

/**
  * @brief 发送心跳帧 0x101（seq 自增由调用方维护）
  * @note  经 BSP_CAN_Send 发送，任务上下文调用
  */
void CanProto_SendHeartbeat(uint8_t seq);

/**
  * @brief 对 0x2FF 请求回 echo 应答 0x2FE（载荷原样回显，首字节为 seq）
  */
void CanProto_SendEchoRsp(const uint8_t *req, uint8_t len);

/**
  * @brief 按 ID 把收到的帧解析为协议报文
  * @return true 识别出的协议帧；false 未知 ID（msg->type = NONE）
  */
bool CanProto_Parse(const BSP_CanFrame *frame, CanProtoMsg *msg);

#ifdef __cplusplus
}
#endif

#endif /* MID_CAN_PROTOCOL_H */
