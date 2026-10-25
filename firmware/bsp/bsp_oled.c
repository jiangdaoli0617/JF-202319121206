#include "bsp_oled.h"

/* TODO: SSD1306 初始化序列、6x8 字库、显存刷新。
 * 实现时只使用 bsp_i2c_write_reg(OLED_ADDR, 0x00/0x40, ...)，
 * 不要把 I2C 细节泄漏到上层。 */

void bsp_oled_init(void) {
}

void bsp_oled_clear(void) {
}

void bsp_oled_write_line(uint8_t line, const char *text) {
    (void)line;
    (void)text;
}

void bsp_oled_refresh(void) {
}
