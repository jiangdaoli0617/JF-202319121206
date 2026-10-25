#ifndef BSP_OLED_H
#define BSP_OLED_H

#include <stdint.h>

/* SSD1306，128x64，I2C 地址 0x3C。
 * 字符接口按 8 行 × 16 列组织，每行最多 21 个 ASCII 字符（6x8 字体）。 */
#define OLED_LINE_COUNT  8U
#define OLED_LINE_WIDTH  21U

void bsp_oled_init(void);
void bsp_oled_clear(void);
void bsp_oled_write_line(uint8_t line, const char *text);
void bsp_oled_refresh(void);

#endif /* BSP_OLED_H */
