/**
  ******************************************************************************
  * @file    BSP/uart/uart.h
  * @brief   日志打印专用串口
  ******************************************************************************
  */
#ifndef BSP_UART_H
#define BSP_UART_H

#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
  * @brief 初始化日志串口
  * @note  串口外设与引脚由 CubeMX 的 MX_USART3_UART_Init() 完成（PB10/PB11），
  *        这里检查 HAL UART 状态是否已经就绪。
  * @return true USART3 就绪；false 状态异常
  */
bool BSP_UART_Init(void);

/**
  * @brief 通过 USART3 发送一段字节（轮询模式）
  * @param data 数据指针
  * @param len  字节长度
  * @return true 发送完成；false 参数错误、超时或 HAL 状态异常
  */
bool BSP_UART_Transmit(const uint8_t *data, uint16_t len);

/**
  * @brief printf 风格的格式化输出，直接走 USART3
  * @param fmt 格式串
  * @retval 写入的字符数（不含终止符），出错时为负
  */
int BSP_UART_Printf(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* BSP_UART_H */
