/**
  ******************************************************************************
  * @file    BSP/uart/uart.c
  * @brief   日志打印专用串口
  ******************************************************************************
  */
#include "BSP/uart/uart.h"

#include "main.h" /* HAL_UART_Transmit */
#include <stdio.h>

#define UART_TX_TIMEOUT_MS 50U

/* USART3 句柄：由 CubeMX 生成（Core/Src/usart.c），这里只引用 */
extern UART_HandleTypeDef huart3;

/**
  * @brief 初始化日志串口
  * @note  外设/引脚初始化由 CubeMX 完成，此处为空实现（BSP 入口对齐）
  */
bool BSP_UART_Init(void)
{
    return HAL_UART_GetState(&huart3) == HAL_UART_STATE_READY;
}

/**
  * @brief 通过 USART3 发送一段字节（轮询模式，超时无限等待）
  */
bool BSP_UART_Transmit(const uint8_t *data, uint16_t len)
{
    if ((data == NULL) || (len == 0U))
    {
        return false;
    }
    return HAL_UART_Transmit(&huart3, (uint8_t *)data, len, UART_TX_TIMEOUT_MS) == HAL_OK;
}

/**
  * @brief printf 风格的格式化输出，直接走 USART3
  */
int BSP_UART_Printf(const char *fmt, ...)
{
    char    buf[128];
    va_list args;
    int     n;

    va_start(args, fmt);
    n = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (n > 0)
    {
        uint16_t len = (n >= (int)sizeof(buf)) ? (uint16_t)(sizeof(buf) - 1U) : (uint16_t)n;
        if (!BSP_UART_Transmit((const uint8_t *)buf, len))
        {
            return -1;
        }
    }
    return n;
}

/**
  * @brief newlib printf 重定向：覆盖 syscalls.c 中的 weak `_write`，
  *        使整个工程的标准 printf() 输出统一走 USART3。
  */
int _write(int file, char *ptr, int len)
{
    (void)file;
    if ((ptr == NULL) || (len <= 0))
    {
        return 0;
    }
    return BSP_UART_Transmit((const uint8_t *)ptr, (uint16_t)len) ? len : -1;
}
