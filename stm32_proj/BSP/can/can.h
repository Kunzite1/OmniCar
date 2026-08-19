/**
  ******************************************************************************
  * @file    BSP/can/can.h
  * @brief   与上位机（KICKPI K1 Mini / RK3568）通信的 CAN 收发驱动
  ******************************************************************************
  */
#ifndef BSP_CAN_H
#define BSP_CAN_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 一帧 CAN 报文（标准数据帧，数据段最多 8 字节） */
typedef struct {
    uint16_t id;      /* 11 位标准 ID */
    uint8_t  len;     /* DLC：0~8 */
    uint8_t  data[8]; /* 数据段（小端） */
} BSP_CanFrame;

/**
  * @brief CAN1 初始化：过滤器全收 → FIFO0，使能接收中断，启动外设
  * @note   外设/引脚/位时序由 CubeMX 配置（PD0/PD1，500 kbps），
  *         此处只做过滤器、中断使能与启动，并创建接收队列
  */
void BSP_CAN_Init(void);

/**
  * @brief 发送一帧标准数据帧（任务上下文调用）
  * @return true 已写入发送邮箱；false 邮箱满（短暂重试后放弃）或参数错
  */
bool BSP_CAN_Send(uint16_t std_id, const uint8_t *data, uint8_t len);

/**
  * @brief 阻塞等待接收一帧（RX0 中断收到的帧经 FreeRTOS 队列转出）
  * @param timeout_ticks 超时（FreeRTOS 节拍）；一直等传 portMAX_DELAY
  * @return true 收到一帧（写入 *frame）；false 超时
 */
bool BSP_CAN_Receive(BSP_CanFrame *frame, uint32_t timeout_ticks);

#ifdef __cplusplus
}
#endif

#endif /* BSP_CAN_H */
