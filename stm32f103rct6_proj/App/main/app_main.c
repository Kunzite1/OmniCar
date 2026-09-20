/**
  ******************************************************************************
  * @file    App/main/app_main.c
 * @brief   业务层入口（初始化 / 主循环）
 *          CAN 链路自检：1 Hz 心跳 + echo 应答，电机保持停止
  ******************************************************************************
  */
#include "App/main/app_main.h"

#include "BSP/can/can.h"
#include "BSP/led/led.h"
#include "BSP/motor/motor.h"
#include "BSP/uart/uart.h"
#include "Middleware/can_protocol/can_protocol.h"
#include "Middleware/log/log.h"
#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "main.h"
#include "task.h"

/* 心跳序号（u8 自然回绕） */
static uint8_t s_heartbeat_seq = 0U;
static bool    s_can_ready;
static bool    s_can_tx_warning_reported;

#if LOG_LEVEL <= LOG_LEVEL_DEBUG
static uint8_t s_health_countdown = 10U;
#endif

/**
  * @brief 业务层初始化：系统时钟和外设初始化完成后调用一次
  */
void App_Init(void)
{
    bool     log_mutex_ok;
    bool     uart_ok;
    bool     motor_ok;
    uint32_t timer_clock;
    uint32_t pwm_frequency;
    uint32_t pwm_prescaler;
    uint32_t pwm_period;

    uart_ok      = BSP_UART_Init();
    log_mutex_ok = Log_Init();

    LOG_INFO("firmware boot, configured log level=%s", LOG_LEVEL_NAME);
    if (!uart_ok)
    {
        LOG_ERROR("USART3 is not ready");
    }
    if (!log_mutex_ok)
    {
        LOG_ERROR("log mutex creation failed; output is not serialized");
    }

    LOG_INFO("clocks sys=%lu hclk=%lu pclk1=%lu pclk2=%lu Hz",
             (unsigned long)HAL_RCC_GetSysClockFreq(),
             (unsigned long)HAL_RCC_GetHCLKFreq(),
             (unsigned long)HAL_RCC_GetPCLK1Freq(),
             (unsigned long)HAL_RCC_GetPCLK2Freq());

    BSP_LED_Init();
    LOG_INFO("USART3 ready at 115200 8N1; PA8 LED initialized off");

    motor_ok     = BSP_Motor_Init(); /* PWM 起来并保持停止，链路自检阶段不转车 */
    pwm_prescaler = BSP_Motor_GetPwmPrescaler();
    pwm_period    = BSP_Motor_GetPwmPeriod();
    timer_clock   = HAL_RCC_GetPCLK1Freq();
    if (HAL_RCC_GetHCLKFreq() != timer_clock)
    {
        timer_clock *= 2U;
    }
    pwm_frequency = timer_clock / ((pwm_prescaler + 1U) * (pwm_period + 1U));
    if (motor_ok)
    {
        LOG_INFO("TIM3 PWM started: psc=%lu arr=%lu freq=%lu Hz; motors stopped",
                 (unsigned long)pwm_prescaler,
                 (unsigned long)pwm_period,
                 (unsigned long)pwm_frequency);
    }
    else
    {
        LOG_ERROR("one or more TIM3 PWM channels failed to start; motors stopped");
    }

    s_can_ready = BSP_CAN_Init();
    if (s_can_ready)
    {
        LOG_INFO("CAN controller started at 500 kbit/s; RX0 interrupt enabled");
    }
    else
    {
        LOG_ERROR("CAN initialization failed; heartbeat transmission disabled");
    }

    LOG_INFO("application initialization complete");
}

/**
  * @brief 业务层主循环体：由 FreeRTOS 默认任务周期调用
  * @note  1 Hz 发心跳帧 0x101（seq 自增）+ LED 翻转；上位机发 0x2FF
  *        测试帧由 canTask 任务回 echo（见 App/cmd_handler）。
  *        上位机侧用 candump can0 看心跳、cansend 发测试帧联调。
  */
void App_Loop(void)
{
    bool queued;

    BSP_LED_Toggle();

    if (s_can_ready)
    {
        queued = CanProto_SendHeartbeat(s_heartbeat_seq++);
        if (!queued && !s_can_tx_warning_reported)
        {
            LOG_WARN("CAN heartbeat could not enter a TX mailbox; bus may have no ACK");
            s_can_tx_warning_reported = true;
        }
    }

#if LOG_LEVEL <= LOG_LEVEL_DEBUG
    if (--s_health_countdown == 0U)
    {
        BSP_CanStats stats = {0};

        if (s_can_ready)
        {
            BSP_CAN_GetStats(&stats);
        }
        LOG_DEBUG("health up=%lus heap=%lu log=%lu tx=%lu/%lu rx=%lu/%lu st=%lu err=%08lX",
                  (unsigned long)(HAL_GetTick() / 1000U),
                  (unsigned long)xPortGetFreeHeapSize(),
                  (unsigned long)Log_GetDroppedCount(),
                  (unsigned long)stats.tx_queued,
                  (unsigned long)stats.tx_failed,
                  (unsigned long)stats.rx_received,
                  (unsigned long)stats.rx_dropped,
                  (unsigned long)stats.state,
                  (unsigned long)stats.error_code);
        s_health_countdown = 10U;
    }
#endif

    osDelay(1000U);
}
