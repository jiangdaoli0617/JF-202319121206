#ifndef BSP_OLED_H
#define BSP_OLED_H

#include <stdint.h>

/* SSD1306，128×64 单色屏，挂在软件模拟 I2C 上（地址见 board_config.h 的 OLED_ADDR）。
 *
 * 字符网格按 8×16 字体组织：整屏分成 4 行（每行 16 像素高），
 * 每行最多 16 个 ASCII 字符，或 8 个 16×16 汉字。 */
#define OLED_WIDTH        128U
#define OLED_HEIGHT       64U
#define OLED_PAGES        (OLED_HEIGHT / 8U)    /* 8 页，每页 8 行像素 */
#define OLED_LINE_COUNT   4U                    /* 64 / 16 = 4 行 */
#define OLED_LINE_WIDTH   16U                   /* 128 / 8  = 16 个 ASCII 字符 */

void bsp_oled_init(void);

/* 以下四个函数配合使用：
 * clear / write_line / write_chinese 只修改内存中的显存镜像，
 * 必须再调用 bsp_oled_refresh() 才会真正写到屏幕上。
 * 这样做的好处是整屏一次性刷新，不会出现画一半闪一下的现象。 */
void bsp_oled_clear(void);
void bsp_oled_refresh(void);

/* 在指定行写一行 ASCII 字符串：超出部分截断，不足部分用空白补齐。
 * line：0~3。 */
void bsp_oled_write_line(uint8_t line, const char *text);

/* 在指定行、指定字符位置显示字库中的第 index 个汉字。
 * col：0~7（一个汉字占两个 ASCII 字符的宽度）。 */
void bsp_oled_write_chinese(uint8_t line, uint8_t col, uint8_t index);

#endif /* BSP_OLED_H */
