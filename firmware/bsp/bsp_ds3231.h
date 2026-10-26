#ifndef BSP_DS3231_H
#define BSP_DS3231_H

#include <stdint.h>

void    bsp_ds3231_init(void);
uint8_t bsp_ds3231_read_raw(uint8_t reg, uint8_t *buf, uint8_t len);
uint8_t bsp_ds3231_write_raw(uint8_t reg, const uint8_t *buf, uint8_t len);
/* 读状态寄存器 OSF 位：1 = 掉电后时间不可信 */
uint8_t bsp_ds3231_oscillator_ok(void);

#endif /* BSP_DS3231_H */
