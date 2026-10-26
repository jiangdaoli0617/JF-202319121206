#include "drv_display.h"
#include "bsp_oled.h"

void drv_display_init(void) {
    bsp_oled_init();
    bsp_oled_clear();
    bsp_oled_refresh();
}

void drv_display_clear(void) {
    bsp_oled_clear();
}

void drv_display_line(uint8_t line, const char *text) {
    bsp_oled_write_line(line, text);
}

void drv_display_refresh(void) {
    bsp_oled_refresh();
}
