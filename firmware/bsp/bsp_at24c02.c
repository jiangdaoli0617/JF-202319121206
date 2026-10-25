#include "bsp_at24c02.h"
#include "bsp_i2c.h"
#include "board_config.h"
#include "delay.h"

#define AT24C02_PAGE_SIZE 8U
#define AT24C02_WRITE_MS  5U

void bsp_at24c02_init(void) {
    /* 无初始化序列。上电后建议先读一个字节确认应答。 */
}

uint8_t bsp_at24c02_read(uint16_t addr, uint8_t *buf, uint16_t len) {
    if (len == 0U || (uint32_t)addr + len > AT24C02_SIZE) {
        return 0U;
    }
    return bsp_i2c_read_reg(AT24C02_ADDR, (uint8_t)addr, buf, len);
}

uint8_t bsp_at24c02_write(uint16_t addr, const uint8_t *buf, uint16_t len) {
    if (len == 0U || (uint32_t)addr + len > AT24C02_SIZE) {
        return 0U;
    }

    /* 按页拆分：写入不能跨 8 B 页边界，否则地址在页内回卷 */
    while (len > 0U) {
        uint16_t page_left = (uint16_t)(AT24C02_PAGE_SIZE - (addr % AT24C02_PAGE_SIZE));
        uint16_t chunk = (len < page_left) ? len : page_left;

        if (!bsp_i2c_write_reg(AT24C02_ADDR, (uint8_t)addr, buf, chunk)) {
            return 0U;
        }
        delay_ms(AT24C02_WRITE_MS);   /* 等待内部写周期结束 */

        addr = (uint16_t)(addr + chunk);
        buf += chunk;
        len = (uint16_t)(len - chunk);
    }
    return 1U;
}
