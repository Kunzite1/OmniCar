/**
  ******************************************************************************
  * @file    BSP/led/led.h
  * @brief   板载 LED（状态指示）
  ******************************************************************************
  */
#ifndef BSP_LED_H
#define BSP_LED_H

#ifdef __cplusplus
extern "C" {
#endif

/**
  * @brief 初始化板载 LED（引脚配置由 CubeMX 完成，这里置为默认熄灭状态）
  */
void BSP_LED_Init(void);

/**
  * @brief 点亮板载 LED（PA1 为开漏输出，低电平点亮）
  */
void BSP_LED_On(void);

/**
  * @brief 熄灭板载 LED
  */
void BSP_LED_Off(void);

/**
  * @brief 翻转板载 LED 状态
  */
void BSP_LED_Toggle(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_LED_H */
