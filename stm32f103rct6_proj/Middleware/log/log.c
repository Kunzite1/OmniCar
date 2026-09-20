/**
  ******************************************************************************
  * @file    Middleware/log/log.c
 * @brief   日志打印中间件（分级 / 过滤 / 时间戳 / 源位置 / 异步队列）
 * @note    LOG_* 只格式化并入队；低优先级日志任务批量调用 BSP/uart 输出。
 *          禁止在中断中调用日志接口。
  ******************************************************************************
  */
#include "Middleware/log/log.h"

#include "BSP/uart/uart.h"
#include "FreeRTOS.h"
#include "main.h" /* HAL_GetTick */
#include "queue.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

#define LOG_BUF_SIZE          192U
#define LOG_PATH_SIZE         96U
#define LOG_QUEUE_LENGTH      16U
#define LOG_TASK_PERIOD_MS    200U
#define LOG_PROJECT_DIR       "stm32f103rct6_proj"

typedef struct
{
    uint16_t length;
    char     text[LOG_BUF_SIZE];
} LogMessage;

static const char *const kLevelTag[] = {
    [LOG_LEVEL_DEBUG] = "DEBUG",
    [LOG_LEVEL_INFO]  = "INFO",
    [LOG_LEVEL_WARN]  = "WARN",
    [LOG_LEVEL_ERROR] = "ERROR",
};

static QueueHandle_t     s_log_queue;
static volatile uint32_t s_dropped_count;

static void Log_RecordDropped(void)
{
    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
    {
        taskENTER_CRITICAL();
        s_dropped_count++;
        taskEXIT_CRITICAL();
    }
    else
    {
        s_dropped_count++;
    }
}

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
    if (s_log_queue != NULL)
    {
        return true;
    }

    s_log_queue = xQueueCreate(LOG_QUEUE_LENGTH, sizeof(LogMessage));
    if (s_log_queue == NULL)
    {
        return false;
    }

    return true;
}

uint32_t Log_GetDroppedCount(void)
{
    return s_dropped_count;
}

/**
  * @brief 低优先级日志消费者：低频唤醒并批量排空队列
  */
void Log_Task(void *argument)
{
    LogMessage message;
    TickType_t last_wake_time;

    (void)argument;
    last_wake_time = xTaskGetTickCount();

    for (;;)
    {
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(LOG_TASK_PERIOD_MS));

        while (xQueueReceive(s_log_queue, &message, 0U) == pdPASS)
        {
            if (!BSP_UART_Transmit((const uint8_t *)message.text, message.length))
            {
                Log_RecordDropped();
            }
        }
    }
}

/**
  * @brief 日志核心函数：组装 [tick] [LEVEL] proj/file:function() + 消息
  */
void Log_Write(int level, const char *file, const char *function, const char *fmt, ...)
{
    LogMessage message;
    char    project_path[LOG_PATH_SIZE];
    va_list args;
    size_t  used;
    int     prefix_len;

    if ((level < LOG_LEVEL) || (level >= LOG_LEVEL_NONE) || (fmt == NULL))
    {
        return;
    }

    Log_MakeProjectPath(file, project_path, sizeof(project_path));
    prefix_len = snprintf(message.text, sizeof(message.text) - 2U, "[%lu] [%s] %s:%s(): ",
                          (unsigned long)HAL_GetTick(),
                          (level >= LOG_LEVEL_DEBUG && level <= LOG_LEVEL_ERROR) ? kLevelTag[level] : "?",
                          project_path,
                          (function != NULL) ? function : "?");
    if (prefix_len < 0)
    {
        return;
    }

    used = (size_t)prefix_len;
    if (used >= sizeof(message.text) - 2U)
    {
        used = sizeof(message.text) - 3U;
    }
    va_start(args, fmt);
    (void)vsnprintf(message.text + used, sizeof(message.text) - used - 2U, fmt, args);
    va_end(args);

    used = strlen(message.text);
    message.text[used++] = '\r';
    message.text[used++] = '\n';
    message.length       = (uint16_t)used;

    if ((s_log_queue == NULL) || (xQueueSend(s_log_queue, &message, 0U) != pdPASS))
    {
        Log_RecordDropped();
    }
}
