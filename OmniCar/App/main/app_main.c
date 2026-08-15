/**
  ******************************************************************************
  * @file    App/main/app_main.c
  * @brief   业务层入口（初始化 / 主循环）
  *          v0.7：电机旋转方向自检——上电自动顺序执行，无需串口指令
  ******************************************************************************
  */
#include "App/main/app_main.h"

#include "BSP/led/led.h"
#include "BSP/motor/motor.h"
#include "BSP/uart/uart.h"
#include "Middleware/log/log.h"
#include "cmsis_os2.h"

/* ---------- v0.7 电机方向自检（顺序执行，循环） ---------- */
typedef struct {
    MotorId     id;    /* 电机编号 */
    MotorDir    dir;   /* 方向 */
    const char *name;  /* 串口日志名 */
} TestStep;

static const TestStep s_steps[] = {
    { MOTOR_1, MOTOR_DIR_FWD, "M1 FWD" },
    { MOTOR_1, MOTOR_DIR_REV, "M1 REV" },
    { MOTOR_2, MOTOR_DIR_FWD, "M2 FWD" },
    { MOTOR_2, MOTOR_DIR_REV, "M2 REV" },
    { MOTOR_3, MOTOR_DIR_FWD, "M3 FWD" },
    { MOTOR_3, MOTOR_DIR_REV, "M3 REV" },
};
#define STEP_NUM ((uint32_t)(sizeof(s_steps) / sizeof(s_steps[0])))

/* 当前执行到第几步（循环） */
static uint32_t s_step = 0;

/**
  * @brief 业务层初始化：系统时钟和外设初始化完成后调用一次
  */
void App_Init(void)
{
    BSP_LED_Init();
    BSP_UART_Init();
    BSP_Motor_Init();

    LOG_INFO("OmniCar v0.7 motor dir selftest start");
}

/**
  * @brief 业务层主循环体：由 FreeRTOS 默认任务周期调用
  * @note  顺序执行每路正转/反转各 3 s（步间停 0.5 s），循环验证旋转方向。
  *        仅单路电机转动，车不会整体移动；看完断电即可。
  */
void App_Loop(void)
{
    BSP_LED_Toggle();

    if (s_step < STEP_NUM)
    {
        const TestStep *st = &s_steps[s_step];

        LOG_INFO("[%u] %s", (unsigned)(s_step + 1U), st->name);
        BSP_Motor_SetDir(st->id, st->dir);
        BSP_Motor_SetDuty(st->id, 30U);
        osDelay(3000U);

        BSP_Motor_Stop(st->id);
        osDelay(500U);

        s_step++;
        if (s_step >= STEP_NUM)
        {
            s_step = 0; /* 循环：方便持续观察，看完断电即可 */
        }
    }
}
