#include "bsp_led.h"

/**
  ******************************************************************************
  * @file    bsp_led.c
  * @brief   板载 LED 驱动（PC13）
  ******************************************************************************
  * 【原理说明】
  * 蓝色小板上的 LED 一端接 3.3V，另一端接 PC13 引脚。
  * 所以 PC13 输出低电平时，电流从 3.3V 经 LED 流入 PC13，LED 点亮；
  * 输出高电平时两端都是 3.3V，没有电流，LED 熄灭。
  * 这个"高低电平与亮滅的对应关系"没有写死在这里，
  * 而是由 board_config.h 里的 LED_ON_LEVEL / LED_OFF_LEVEL 决定，
  * 万一日后换了高电平点亮的板子，只改那两行宏即可。
  ******************************************************************************
  */

/**
  * @brief  LED 初始化
  */
void LED_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    /* 第一步：打开 LED 所在端口的时钟。STM32 的外设不打开时钟就不工作！ */
    RCC_APB2PeriphClockCmd(LED_CLK, ENABLE);

    /* 第二步：把 LED 引脚配置成推挽输出 */
    GPIO_InitStruct.GPIO_Pin   = LED_PIN;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_PP;      /* 推挽输出 */
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;      /* 输出速度 50MHz */
    GPIO_Init(LED_PORT, &GPIO_InitStruct);

    /* 第三步：上电默认熄灭，避免一上电就常亮让人误以为在运行 */
    LED_Off();
}

/**
  * @brief  点亮 LED
  */
void LED_On(void)
{
    GPIO_WriteBit(LED_PORT, LED_PIN, (BitAction)LED_ON_LEVEL);
}

/**
  * @brief  熄灭 LED
  */
void LED_Off(void)
{
    GPIO_WriteBit(LED_PORT, LED_PIN, (BitAction)LED_OFF_LEVEL);
}

/**
  * @brief  翻转 LED 状态
  */
void LED_Toggle(void)
{
    /* 读回当前输出电平，是低就熄灭，是高就点亮 */
    if (GPIO_ReadOutputDataBit(LED_PORT, LED_PIN) == Bit_RESET)
    {
        LED_Off();
    }
    else
    {
        LED_On();
    }
}
