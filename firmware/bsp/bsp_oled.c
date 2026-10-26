#include "bsp_oled.h"
#include "bsp_oled_font.h"
#include "bsp_i2c.h"
#include "board_config.h"

#include <string.h>

/* ============================================================================
 * SSD1306 128×64 OLED 驱动（I2C 接口）
 *
 * 显存组织：
 *   屏幕被横向切成 8 页（page），每页 8 行像素、128 列。
 *   显存里一个字节 = 某一页某一列的 8 个像素，最低位是最上面那行。
 *   所以整屏显存 = 8 页 × 128 列 = 1024 字节。
 *
 * 本驱动先在内存里维护一份 1024 字节的显存镜像 s_gram，
 * 所有绘制都改镜像，调用 bsp_oled_refresh() 时才一次性推给屏幕。
 * ============================================================================ */

/* SSD1306 的控制字节：紧跟从机地址的那个字节决定后面是命令还是数据 */
#define OLED_CTRL_CMD        0x00U      /* 后面全部是命令 */
#define OLED_CTRL_DATA       0x40U      /* 后面全部是显存数据 */

/* 显存镜像：128 列 × 8 页 */
static uint8_t s_gram[OLED_WIDTH * OLED_PAGES];

/* 空白字符，用于把一行里没有内容的列擦干净 */
static const uint8_t BLANK_8x16[16] = { 0U };

/* SSD1306 上电初始化命令序列，含义见右侧注释 */
static const uint8_t OLED_INIT_SEQ[] = {
    0xAEU,              /* 关显示，避免初始化过程中出现花屏 */
    0xD5U, 0x80U,       /* 时钟分频因子 */
    0xA8U, 0x3FU,       /* 复用率 1/64，对应 64 行 */
    0xD3U, 0x00U,       /* 显示偏移 = 0 */
    0x40U,              /* 显示起始行 = 0 */
    0x8DU, 0x14U,       /* 开启内部电荷泵（3.3V 供电时必须开，否则不亮） */
    0x20U, 0x00U,       /* 水平寻址模式 */
    0xA1U,              /* 列地址反向，保证左右方向不镜像 */
    0xC8U,              /* COM 扫描反向，保证上下方向不镜像 */
    0xDAU, 0x12U,       /* COM 引脚硬件配置 */
    0x81U, 0xCFU,       /* 对比度（亮度） */
    0xD9U, 0xF1U,       /* 预充电周期 */
    0xDBU, 0x30U,       /* VCOMH 电压 */
    0xA4U,              /* 显示内容跟随显存 */
    0xA6U,              /* 正常显示（改成 0xA7 就是反白） */
    0x2EU,              /* 关闭滚动 */
    0xAFU               /* 开显示 */
};

/* 发一条命令 */
static void oled_write_cmd(uint8_t cmd) {
    (void)bsp_i2c_write_reg(OLED_ADDR, OLED_CTRL_CMD, &cmd, 1U);
}

/* 设定后续显存数据的写入范围：整个屏幕（第 0~7 页、第 0~127 列） */
static void oled_set_window_full(void) {
    uint8_t buf[3];

    buf[0] = 0x21U;                                 /* 设置列地址范围 */
    buf[1] = 0x00U;
    buf[2] = (uint8_t)(OLED_WIDTH - 1U);
    (void)bsp_i2c_write_reg(OLED_ADDR, OLED_CTRL_CMD, buf, 3U);

    buf[0] = 0x22U;                                 /* 设置页地址范围 */
    buf[1] = 0x00U;
    buf[2] = (uint8_t)(OLED_PAGES - 1U);
    (void)bsp_i2c_write_reg(OLED_ADDR, OLED_CTRL_CMD, buf, 3U);
}

/* 把一个 8×16 字符的字模画进显存镜像。
 * glyph 前 8 字节画到上半页，后 8 字节画到下半页。 */
static void draw_glyph_8x16(uint8_t line, uint8_t col, const uint8_t *glyph) {
    uint16_t x   = (uint16_t)((uint16_t)col * 8U);
    uint16_t top = (uint16_t)((uint16_t)line * 2U * OLED_WIDTH + x);
    uint16_t bottom = (uint16_t)(top + OLED_WIDTH);
    uint8_t  i;

    for (i = 0U; i < 8U; i++) {
        s_gram[top + i]    = glyph[i];
        s_gram[bottom + i] = glyph[8U + i];
    }
}

void bsp_oled_init(void) {
    (void)bsp_i2c_write_reg(OLED_ADDR, OLED_CTRL_CMD,
                            OLED_INIT_SEQ, (uint16_t)sizeof(OLED_INIT_SEQ));

    bsp_oled_clear();
    bsp_oled_refresh();
}

void bsp_oled_clear(void) {
    memset(s_gram, 0, sizeof(s_gram));
}

void bsp_oled_refresh(void) {
    oled_set_window_full();

    /* 1024 字节一次连续写完。100kHz 总线速率下约耗时 90ms，
     * 因此只应在界面内容变化时调用，不要在 while(1) 里每圈都刷。 */
    (void)bsp_i2c_write_reg(OLED_ADDR, OLED_CTRL_DATA,
                            s_gram, (uint16_t)sizeof(s_gram));
}

void bsp_oled_write_line(uint8_t line, const char *text) {
    uint8_t col;

    if (line >= OLED_LINE_COUNT || text == 0) {
        return;
    }

    /* 逐字符画到显存镜像里，遇到字符串结束或超出该行宽度就停 */
    for (col = 0U; col < OLED_LINE_WIDTH && text[col] != '\0'; col++) {
        uint8_t ch = (uint8_t)text[col];

        if (ch < 0x20U || ch > 0x7EU) {
            ch = (uint8_t)' ';              /* 非可见字符按空格处理 */
        }
        draw_glyph_8x16(line, col, OLED_F8x16[ch - 0x20U]);
    }

    /* 后面的列擦成空白，避免上一次显示的内容残留 */
    for (; col < OLED_LINE_WIDTH; col++) {
        draw_glyph_8x16(line, col, BLANK_8x16);
    }
}

void bsp_oled_write_chinese(uint8_t line, uint8_t col, uint8_t index) {
    const uint8_t *glyph;
    uint16_t x;
    uint16_t top;
    uint16_t bottom;
    uint8_t  i;

    if (line >= OLED_LINE_COUNT || col >= (OLED_WIDTH / 16U)) {
        return;
    }
    if (index >= OLED_CN_FONT_COUNT) {
        return;
    }

    glyph  = OLED_F16x16[index];
    x      = (uint16_t)((uint16_t)col * 16U);
    top    = (uint16_t)((uint16_t)line * 2U * OLED_WIDTH + x);
    bottom = (uint16_t)(top + OLED_WIDTH);

    /* 汉字宽 16 列，同样分上下两页各 16 字节 */
    for (i = 0U; i < 16U; i++) {
        s_gram[top + i]    = glyph[i];
        s_gram[bottom + i] = glyph[16U + i];
    }
}
