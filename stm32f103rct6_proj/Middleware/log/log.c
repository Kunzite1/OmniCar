/**
  ******************************************************************************
  * @file    Middleware/log/log.c
 * @brief   日志打印中间件（分级 / 过滤 / 时间戳 / 源位置）
 * @note    输出走 BSP/uart 串口驱动，时间戳取 HAL_GetTick()；任务上下文
 *          通过 FreeRTOS 互斥量串行输出。禁止在中断中调用日志接口。
  ******************************************************************************
  */
#include "Middleware/log/log.h"

#include "BSP/uart/uart.h"
#include "FreeRTOS.h"
#include "main.h" /* HAL_GetTick */
#include "semphr.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

#define LOG_BUF_SIZE       192U
#define LOG_PATH_SIZE      96U
#define LOG_MUTEX_WAIT_MS  25U
#define LOG_PROJECT_DIR    "stm32f103rct6_proj"

static const char *const kLevelTag[] = {
    [LOG_LEVEL_DEBUG] = "DEBUG",
    [LOG_LEVEL_INFO]  = "INFO",
    [LOG_LEVEL_WARN]  = "WARN",
    [LOG_LEVEL_ERROR] = "ERROR",
};

static SemaphoreHandle_t s_log_mutex;
static volatile uint32_t s_dropped_count;

/**
  * @brief 把编译器传入的绝对路径裁剪并标准化为 proj/...
  */
static void Log_MakeProjectPath(const char *file, char *output, size_t output_size)
{
    const char *relative;
    const char *basename;
    size_t      i;

    if ((file == NULL) || (output == NULL) || (output_size == 0U))
    {
        return;
    }

    relative = strstr(file, LOG_PROJECT_DIR "/");
    if (relative != NULL)
    {
        relative += strlen(LOG_PROJECT_DIR) + 1U;
    }
    else
    {
        relative = strstr(file, LOG_PROJECT_DIR "\\");
        if (relative != NULL)
        {
            relative += strlen(LOG_PROJECT_DIR) + 1U;
        }
    }

    if (relative == NULL)
    {
        const char *slash     = strrchr(file, '/');
        const char *backslash = strrchr(file, '\\');

        basename = file;
        if ((slash != NULL) && (slash + 1 > basename))
        {
            basename = slash + 1;
        }
        if ((backslash != NULL) && (backslash + 1 > basename))
        {
            basename = backslash + 1;
        }
        relative = basename;
    }

    (void)snprintf(output, output_size, "proj/%s", relative);
    output[output_size - 1U] = '\0';
    for (i = 0U; output[i] != '\0'; i++)
    {
        if (output[i] == '\\')
        {
            output[i] = '/';
        }
    }
}

bool Log_Init(void)
{
    if (s_log_mutex == NULL)
    {
        s_log_mutex = xSemaphoreCreateMutex();
    }
    return s_log_mutex != NULL;
}

uint32_t Log_GetDroppedCount(void)
{
    return s_dropped_count;
}

/**
  * @brief 日志核心函数：组装 [tick] [LEVEL] proj/file:function() + 消息
  */
void Log_Write(int level, const char *file, const char *function, const char *fmt, ...)
{
    char    buf[LOG_BUF_SIZE];
    char    project_path[LOG_PATH_SIZE];
    va_list args;
    size_t  used;
    int     prefix_len;
    bool    locked = false;

    if ((level < LOG_LEVEL) || (level >= LOG_LEVEL_NONE) || (fmt == NULL))
    {
        return;
    }

    Log_MakeProjectPath(file, project_path, sizeof(project_path));
    prefix_len = snprintf(buf, sizeof(buf) - 2U, "[%lu] [%s] %s:%s(): ",
                          (unsigned long)HAL_GetTick(),
                          (level >= LOG_LEVEL_DEBUG && level <= LOG_LEVEL_ERROR) ? kLevelTag[level] : "?",
                          project_path,
                          (function != NULL) ? function : "?");
    if (prefix_len < 0)
    {
        return;
    }

    used = (size_t)prefix_len;
    if (used >= sizeof(buf) - 2U)
    {
        used = sizeof(buf) - 3U;
    }
    va_start(args, fmt);
    (void)vsnprintf(buf + used, sizeof(buf) - used - 2U, fmt, args);
    va_end(args);

    used = strlen(buf);
    buf[used++] = '\r';
    buf[used++] = '\n';

    /* 调度器启动前没有任务并发，也不能取得 mutex（此时尚无当前任务）。 */
    if ((s_log_mutex != NULL) && (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING))
    {
        if (xSemaphoreTake(s_log_mutex, pdMS_TO_TICKS(LOG_MUTEX_WAIT_MS)) != pdTRUE)
        {
            s_dropped_count++;
            return;
        }
        locked = true;
    }

    if (!BSP_UART_Transmit((const uint8_t *)buf, (uint16_t)used))
    {
        s_dropped_count++;
    }

    if (locked)
    {
        (void)xSemaphoreGive(s_log_mutex);
    }
}
