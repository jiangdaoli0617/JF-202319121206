#include "bsp_at24c256.h"
#include "bsp_i2c.h"
#include "board_config.h"
#include "delay.h"

void bsp_at24c256_init(void) {
    /* AT24C256 没有初始化序列，上电后能应答即可工作。
     * 需要确认器件在位时调用 bsp_at24c256_probe()。 */
}

uint8_t bsp_at24c256_probe(void) {
    uint8_t dummy;

    return bsp_i2c_read_mem16(AT24C256_ADDR, 0x0000U, &dummy, 1U);
}

uint8_t bsp_at24c256_read(uint16_t addr, uint8_t *buf, uint16_t len) {
    if ((len == 0U) || (buf == 0) ||
        ((uint32_t)addr + (uint32_t)len > AT24C256_SIZE)) {
        return 0U;
    }
    return bsp_i2c_read_mem16(AT24C256_ADDR, addr, buf, len);
}

uint8_t bsp_at24c256_write(uint16_t addr, const uint8_t *buf, uint16_t len) {
    if ((len == 0U) || (buf == 0) ||
        ((uint32_t)addr + (uint32_t)len > AT24C256_SIZE)) {
        return 0U;
    }

    /* 按页拆分：写入不能跨 64 B 页边界，否则地址在页内回卷、数据被覆盖。
     * 每写完一页要等 5 ms 内部写周期结束，期间器件不应答。 */
    while (len > 0U) {
        uint16_t page_left = (uint16_t)(AT24C256_PAGE_SIZE -
                                        (addr % AT24C256_PAGE_SIZE));
        uint16_t chunk = (len < page_left) ? len : page_left;

        if (!bsp_i2c_write_mem16(AT24C256_ADDR, addr, buf, chunk)) {
            return 0U;
        }
        delay_ms(AT24C256_WRITE_MS);

        addr = (uint16_t)(addr + chunk);
        buf += chunk;
        len = (uint16_t)(len - chunk);
    }
    return 1U;
}
