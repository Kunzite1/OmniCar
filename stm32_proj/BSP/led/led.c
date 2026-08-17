/**
  ******************************************************************************
  * @file    BSP/led/led.c
  * @brief   板载 LED（状态指示）
  ******************************************************************************
  */
#include "BSP/led/led.h"

#include "main.h" /* boardLED_Pin / boardLED_GPIO_Port */

/**
  * @brief 初始化板载 LED（引脚配置由 CubeMX 完成，这里置为默认熄灭状态）
  */
void BSP_LED_Init(void)
{
    BSP_LED_Off();
}

/**
  * @brief 点亮板载 LED（PA1 为开漏输出，低电平点亮）
  */
void BSP_LED_On(void)
{
    HAL_GPIO_WritePin(boardLED_GPIO_Port, boardLED_Pin, GPIO_PIN_RESET);
}

/**
  * @brief 熄灭板载 LED
  */
void BSP_LED_Off(void)
{
    HAL_GPIO_WritePin(boardLED_GPIO_Port, boardLED_Pin, GPIO_PIN_SET);
}

/**
  * @brief 翻转板载 LED 状态
  */
void BSP_LED_Toggle(void)
{
    HAL_GPIO_TogglePin(boardLED_GPIO_Port, boardLED_Pin);
}
