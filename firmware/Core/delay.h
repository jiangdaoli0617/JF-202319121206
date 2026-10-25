#ifndef DELAY_H
#define DELAY_H

#include <stdint.h>

/* 基于 DWT 周期计数器，精度与主频无关，不占用定时器。
 * 仅在初始化阶段与设备驱动内部使用，主循环中禁止长延时。 */
void delay_init(void);
void delay_us(uint32_t us);
void delay_ms(uint32_t ms);

#endif /* DELAY_H */
