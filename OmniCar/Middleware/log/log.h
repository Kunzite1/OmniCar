/**
  ******************************************************************************
  * @file    Middleware/log/log.h
  * @brief   日志打印中间件（分级 / 过滤 / 时间戳）
  ******************************************************************************
  */
#ifndef MIDDLEWARE_LOG_H
#define MIDDLEWARE_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

/* 日志级别（值越大越严重） */
#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO  1
#define LOG_LEVEL_WARN  2
#define LOG_LEVEL_ERROR 3
#define LOG_LEVEL_NONE  4 /* 全部关闭 */

/* 全局输出级别（编译期设定）：低于该级别的日志不参与输出 */
#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_INFO
#endif

/**
  * @brief 日志核心函数：组装 [tick] [LEVEL] file:line + 消息 后输出
  * @note  由 LOG_* 宏调用，一般无需直接使用
  */
void Log_Write(int level, const char *file, int line, const char *fmt, ...);

/* 各级别宏：附带源文件与行号；低于全局级别的调用在编译期被剔除 */
#if LOG_LEVEL <= LOG_LEVEL_DEBUG
#define LOG_DEBUG(...) Log_Write(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#else
#define LOG_DEBUG(...) ((void)0)
#endif

#if LOG_LEVEL <= LOG_LEVEL_INFO
#define LOG_INFO(...) Log_Write(LOG_LEVEL_INFO, __FILE__, __LINE__, __VA_ARGS__)
#else
#define LOG_INFO(...) ((void)0)
#endif

#if LOG_LEVEL <= LOG_LEVEL_WARN
#define LOG_WARN(...) Log_Write(LOG_LEVEL_WARN, __FILE__, __LINE__, __VA_ARGS__)
#else
#define LOG_WARN(...) ((void)0)
#endif

/* ERROR 始终输出 */
#define LOG_ERROR(...) Log_Write(LOG_LEVEL_ERROR, __FILE__, __LINE__, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* MIDDLEWARE_LOG_H */
