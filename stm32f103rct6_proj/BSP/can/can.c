/**
  ******************************************************************************
  * @file    BSP/can/can.c
  * @brief   与上位机（KICKPI K1 Mini / RK3568）通信的 CAN 收发驱动
  * @note    位时序/引脚由 CubeMX 配置（PA11/PA12，500 kbps，APB1 36 MHz）；
  *          接收路径：FIFO0 消息挂起中断 → 复制入队（FromISR）→
  *          任务侧 BSP_CAN_Receive() 阻塞取帧
  ******************************************************************************
  */
#include "BSP/can/can.h"

#include "FreeRTOS.h"
#include "main.h" /* HAL CAN 类型（注意：不能 include "can.h"——会被同目录的 BSP 头抢占） */
#include "queue.h"

/* CAN1 句柄：由 CubeMX 生成（Core/Src/can.c），这里只引用 */
extern CAN_HandleTypeDef hcan;

/* 接收帧队列长度：中断生产者、任务消费者 */
#define CAN_RX_QUEUE_LEN 16U

/* 发送邮箱满时的重试次数（每次让出 1 个节拍） */
#define CAN_TX_RETRY 3U

static QueueHandle_t s_rx_queue;
static volatile uint32_t s_tx_queued;
static volatile uint32_t s_tx_failed;
static volatile uint32_t s_rx_received;
static volatile uint32_t s_rx_dropped;

/**
  * @brief 初始化 CAN1：过滤器全收 → FIFO0，使能接收中断并启动
 */
bool BSP_CAN_Init(void)
{
    CAN_FilterTypeDef filter = {0};

    /* 过滤器 0：32 位掩码模式，ID/掩码全 0 = 全接收，路由到 FIFO0 */
    filter.FilterBank           = 0;
    filter.FilterMode           = CAN_FILTERMODE_IDMASK;
    filter.FilterScale          = CAN_FILTERSCALE_32BIT;
    filter.FilterIdHigh         = 0x0000U;
    filter.FilterIdLow          = 0x0000U;
    filter.FilterMaskIdHigh     = 0x0000U;
    filter.FilterMaskIdLow      = 0x0000U;
    filter.FilterFIFOAssignment = CAN_RX_FIFO0;
    filter.FilterActivation     = ENABLE;
    filter.SlaveStartFilterBank = 14;
    if (HAL_CAN_ConfigFilter(&hcan, &filter) != HAL_OK)
    {
        return false;
    }

    s_rx_queue = xQueueCreate(CAN_RX_QUEUE_LEN, sizeof(BSP_CanFrame));
    if (s_rx_queue == NULL)
    {
        return false;
    }

    if (HAL_CAN_Start(&hcan) != HAL_OK)
    {
        vQueueDelete(s_rx_queue);
        s_rx_queue = NULL;
        return false;
    }
    if (HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
    {
        (void)HAL_CAN_Stop(&hcan);
        vQueueDelete(s_rx_queue);
        s_rx_queue = NULL;
        return false;
    }
    return true;
}

/**
  * @brief 发送一帧标准数据帧（任务上下文调用）
 */
bool BSP_CAN_Send(uint16_t std_id, const uint8_t *data, uint8_t len)
{
    CAN_TxHeaderTypeDef tx;
    uint32_t            mailbox;
    uint32_t            retry;

    if ((data == NULL) || (len > 8U) || (std_id > 0x7FFU))
    {
        s_tx_failed++;
        return false;
    }

    tx.StdId             = std_id;
    tx.ExtId             = 0U;
    tx.IDE               = CAN_ID_STD;
    tx.RTR               = CAN_RTR_DATA;
    tx.DLC               = len;
    tx.TransmitGlobalTime = DISABLE;

    for (retry = 0U; retry < CAN_TX_RETRY; retry++)
    {
        if (HAL_CAN_AddTxMessage(&hcan, &tx, (uint8_t *)data, &mailbox) == HAL_OK)
        {
            s_tx_queued++;
            return true;
        }
        vTaskDelay(1U); /* 邮箱满：等上一帧发走 */
    }
    s_tx_failed++;
    return false;
}

/**
  * @brief 阻塞等待接收一帧
 */
bool BSP_CAN_Receive(BSP_CanFrame *frame, uint32_t timeout_ticks)
{
    if ((frame == NULL) || (s_rx_queue == NULL))
    {
        return false;
    }
    return xQueueReceive(s_rx_queue, frame, timeout_ticks) == pdTRUE;
}

/**
  * @brief HAL 回调（FIFO0 消息挂起中断，IRQ 上下文）：
  *        取出报文后从中断安全入队，唤醒等待的任务
 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef rx;
    BSP_CanFrame        frame;
    BaseType_t          woken = pdFALSE;

    if ((hcan == NULL) || (hcan->Instance != CAN1) || (s_rx_queue == NULL))
    {
        s_rx_dropped++;
        return;
    }

    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx, frame.data) == HAL_OK)
    {
        if ((rx.IDE != CAN_ID_STD) || (rx.RTR != CAN_RTR_DATA) || (rx.DLC > 8U))
        {
            s_rx_dropped++;
            return;
        }
        frame.id  = (uint16_t)rx.StdId;
        frame.len = (uint8_t)rx.DLC;
        if (xQueueSendFromISR(s_rx_queue, &frame, &woken) == pdTRUE)
        {
            s_rx_received++;
        }
        else
        {
            s_rx_dropped++;
        }
    }
    else
    {
        s_rx_dropped++;
    }
    portYIELD_FROM_ISR(woken);
}

void BSP_CAN_GetStats(BSP_CanStats *stats)
{
    if (stats == NULL)
    {
        return;
    }

    taskENTER_CRITICAL();
    stats->tx_queued   = s_tx_queued;
    stats->tx_failed   = s_tx_failed;
    stats->rx_received = s_rx_received;
    stats->rx_dropped  = s_rx_dropped;
    taskEXIT_CRITICAL();

    stats->error_code = HAL_CAN_GetError(&hcan);
    stats->state      = (uint32_t)HAL_CAN_GetState(&hcan);
}
