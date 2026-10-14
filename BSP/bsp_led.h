#ifndef __BSP_LED_H
#define __BSP_LED_H

/**
  ******************************************************************************
  * @file    bsp_led.h
  * @brief   板载 LED 驱动（PC13）
  * @note    引脚定义不在这里写死，全部去 board_config.h 里取。
  ******************************************************************************
  */

#include "stm32f10x.h"
#include "board_config.h"


/* ------------------------------ 对外函数 ------------------------------ */

/**
  * @brief  LED 初始化
  * @note   把 LED 引脚配置成推挽输出，并默认熄灭。
  */
void LED_Init(void);

/**
  * @brief  点亮 LED
  */
void LED_On(void);

/**
  * @brief  熄灭 LED
  */
void LED_Off(void);

/**
  * @brief  翻转 LED 状态（亮变灭、灭变亮）
  * @note   常用于做"系统正在运行"的心跳指示。
  */
void LED_Toggle(void);


#endif /* __BSP_LED_H */
