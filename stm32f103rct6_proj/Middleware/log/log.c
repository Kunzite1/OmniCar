/**
  ******************************************************************************
  * @file    Middleware/log/log.c
  * @brief   日志打印中间件（分级 / 过滤 / 时间戳）
  * @note    输出直接走 BSP/uart 串口驱动，时间戳取 HAL_GetTick()；
  *          本模块属于"简单封装"，不做注册回调等间接机制。
  ******************************************************************************
  */
#include "Middleware/log/log.h"

#include "BSP/uart/uart.h"
#include "main.h" /* HAL_GetTick */
#include <stdio.h>
#include <string.h>

#define LOG_BUF_SIZE 128

static const char *const kLevelTag[] = {
    [LOG_LEVEL_DEBUG] = "DEBUG",
    [LOG_LEVEL_INFO]  = "INFO",
    [LOG_LEVEL_WARN]  = "WARN",
    [LOG_LEVEL_ERROR] = "ERROR",
};

/**
  * @brief 日志核心函数：组装 [tick] [LEVEL] file:line + 消息 后输出
  */
void Log_Write(int level, const char *file, int line, const char *fmt, ...)
{
    char    buf[LOG_BUF_SIZE];
    va_list args;
    uint16_t len;

    /* 裁剪到固件工程根：隐藏编译机中的绝对路径前缀 */
    const char *root = strstr(file, "/stm32_proj/");
    if (root != NULL)
    {
        file = root + 1;
    }

    /* 前缀：时间戳 + 级别 + 源位置 */
    snprintf(buf, sizeof(buf), "[%lu] [%s] %s:%d ",
             (unsigned long)HAL_GetTick(),
             (level >= 0 && level <= LOG_LEVEL_ERROR) ? kLevelTag[level] : "?",
             file, line);

    /* 正文：用户消息（自动截断防止越界） */
    va_start(args, fmt);
    vsnprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), fmt, args);
    va_end(args);

    len = (uint16_t)strlen(buf);
    BSP_UART_Transmit((const uint8_t *)buf, len);
    BSP_UART_Transmit((const uint8_t *)"\r\n", 2);
}
