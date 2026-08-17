/**
  ******************************************************************************
  * @file    BSP/uart/uart.h
  * @brief   日志打印专用串口
  ******************************************************************************
  */
#ifndef BSP_UART_H
#define BSP_UART_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdarg.h>

/**
  * @brief 初始化日志串口
  * @note  串口外设与引脚由 CubeMX 的 MX_USART2_UART_Init() 完成，
  *        这里仅作为 BSP 层入口预留（当前为空实现）。
  */
void BSP_UART_Init(void);

/**
  * @brief 通过 USART2 发送一段字节（轮询模式）
  * @param data 数据指针
  * @param len  字节长度
  */
void BSP_UART_Transmit(const uint8_t *data, uint16_t len);

/**
  * @brief printf 风格的格式化输出，直接走 USART2
  * @param fmt 格式串
  * @retval 写入的字符数（不含终止符），出错时为负
  */
int BSP_UART_Printf(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* BSP_UART_H */
