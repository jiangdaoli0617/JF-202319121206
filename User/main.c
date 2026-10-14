/**
  ******************************************************************************
  * @file    main.c
  * @brief   【模块1 测试程序】SysTick 延时 + 板载 LED 闪烁
  * @note    本模块验证三件事：
  *          1) 系统时钟是否正确跑在 72MHz（延时准不准）
  *          2) GPIO 输出是否正常
  *          3) SysTick 配置是否正确
  *          只有这个模块上机通过后，才继续开发 OLED 模块。
  ******************************************************************************
  */

#include "stm32f10x.h"
#include "board_config.h"
#include "bsp_delay.h"
#include "bsp_led.h"


int main(void)
{
    /* 第一步：初始化延时模块，后面所有模块都依赖它 */
    Delay_Init();

    /* 第二步：初始化 LED 引脚 */
    LED_Init();

    /* 第三步：主循环 —— LED 亮 0.5 秒、灭 0.5 秒，周期 1 秒 */
    while (1)
    {
        LED_On();
        Delay_Ms(500);

        LED_Off();
        Delay_Ms(500);
    }
}
