#ifndef BSP_GPIO_H
#define BSP_GPIO_H

#include <stdint.h>

void    bsp_gpio_init(void);
/* index: 0~3 对应 K1~K4；返回 1 表示按下（低电平有效） */
uint8_t bsp_key_raw(uint8_t index);
void    bsp_led_green(uint8_t on);
void    bsp_led_red(uint8_t on);
void    bsp_beep(uint8_t on);

#endif /* BSP_GPIO_H */
