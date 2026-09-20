/**
  ******************************************************************************
  * @file    Middleware/log/log.h
 * @brief   日志打印中间件（分级 / 过滤 / 时间戳 / 源位置）
  ******************************************************************************
  */
#ifndef MIDDLEWARE_LOG_H
#define MIDDLEWARE_LOG_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 日志级别（值越大越严重） */
#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO  1
#define LOG_LEVEL_WARN  2
#define LOG_LEVEL_ERROR 3
#define LOG_LEVEL_NONE  4 /* 全部关闭 */

/*
 * 全局日志等级，只需修改下面这一处：
 *
 *   LOG_LEVEL_DEBUG  详细调试 + 10 秒健康摘要
 *   LOG_LEVEL_INFO   启动和关键事件（推荐日常使用）
 *   LOG_LEVEL_WARN   仅可恢复异常与错误
 *   LOG_LEVEL_ERROR  仅严重错误
 *   LOG_LEVEL_NONE   完全关闭，包括 ERROR
 *
 * 低于该等级的日志在编译期移除，不占用串口时间，也不会保留格式串。
 */
#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_INFO /* 首次板级测试；完成后可改回 LOG_LEVEL_INFO */
#endif

#if (LOG_LEVEL < LOG_LEVEL_DEBUG) || (LOG_LEVEL > LOG_LEVEL_NONE)
#error "LOG_LEVEL must be LOG_LEVEL_DEBUG, INFO, WARN, ERROR or NONE"
#endif

#if LOG_LEVEL == LOG_LEVEL_DEBUG
#define LOG_LEVEL_NAME "DEBUG"
#elif LOG_LEVEL == LOG_LEVEL_INFO
#define LOG_LEVEL_NAME "INFO"
#elif LOG_LEVEL == LOG_LEVEL_WARN
#define LOG_LEVEL_NAME "WARN"
#elif LOG_LEVEL == LOG_LEVEL_ERROR
#define LOG_LEVEL_NAME "ERROR"
#else
#define LOG_LEVEL_NAME "NONE"
#endif

/**
  * @brief 初始化日志串口互斥量
  * @note  在 osKernelInitialize() 之后、任务创建之前调用；失败时仍可输出，
  *        但任务间不再具备串行化保护。
  * @return true 初始化成功；false 互斥量创建失败
  */
bool Log_Init(void);

/**
  * @brief 获取因互斥超时或 USART3 发送失败而丢弃的日志数量
  */
uint32_t Log_GetDroppedCount(void);

/**
  * @brief 日志核心函数：组装 [tick] [LEVEL] proj/file:function() + 消息
  * @note  由 LOG_* 宏调用，一般无需直接使用
  */
void Log_Write(int level, const char *file, const char *function, const char *fmt, ...);

/* 各级别宏：附带工程相对路径与函数名；低于全局级别的调用在编译期剔除。 */
#if LOG_LEVEL <= LOG_LEVEL_DEBUG
#define LOG_DEBUG(...) Log_Write(LOG_LEVEL_DEBUG, __FILE__, __func__, __VA_ARGS__)
#else
#define LOG_DEBUG(...) ((void)0)
#endif

#if LOG_LEVEL <= LOG_LEVEL_INFO
#define LOG_INFO(...) Log_Write(LOG_LEVEL_INFO, __FILE__, __func__, __VA_ARGS__)
#else
#define LOG_INFO(...) ((void)0)
#endif

#if LOG_LEVEL <= LOG_LEVEL_WARN
#define LOG_WARN(...) Log_Write(LOG_LEVEL_WARN, __FILE__, __func__, __VA_ARGS__)
#else
#define LOG_WARN(...) ((void)0)
#endif

#if LOG_LEVEL <= LOG_LEVEL_ERROR
#define LOG_ERROR(...) Log_Write(LOG_LEVEL_ERROR, __FILE__, __func__, __VA_ARGS__)
#else
#define LOG_ERROR(...) ((void)0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* MIDDLEWARE_LOG_H */
