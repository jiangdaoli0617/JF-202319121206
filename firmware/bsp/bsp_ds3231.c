#include "bsp_ds3231.h"
#include "bsp_i2c.h"
#include "board_config.h"

#define DS3231_REG_STATUS  0x0F
#define DS3231_OSF_MASK    0x80U

void bsp_ds3231_init(void) {
    /* DS3231 无需初始化寄存器；掉电后首读需检查 OSF。
     * 具体的时间结构转换在 drv_rtc 中完成。 */
}

uint8_t bsp_ds3231_read_raw(uint8_t reg, uint8_t *buf, uint8_t len) {
    return bsp_i2c_read_reg(DS3231_ADDR, reg, buf, len);
}

uint8_t bsp_ds3231_write_raw(uint8_t reg, const uint8_t *buf, uint8_t len) {
    return bsp_i2c_write_reg(DS3231_ADDR, reg, buf, len);
}

uint8_t bsp_ds3231_oscillator_ok(void) {
    uint8_t status;

    if (!bsp_ds3231_read_raw(DS3231_REG_STATUS, &status, 1U)) {
        return 0U;
    }
    return ((status & DS3231_OSF_MASK) == 0U) ? 1U : 0U;
}
