#ifndef DRV_DISPLAY_H
#define DRV_DISPLAY_H

#include <stdint.h>

void drv_display_init(void);
void drv_display_clear(void);
/* line: 0~3，每行最多 16 个 ASCII 字符（8×16 字体） */
void drv_display_line(uint8_t line, const char *text);
void drv_display_refresh(void);

#endif /* DRV_DISPLAY_H */
