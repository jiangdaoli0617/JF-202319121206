#ifndef BSP_AT24C256_H
#define BSP_AT24C256_H

#include <stdint.h>

/* ============================================================================
 * AT24C256 —— 256 Kbit（32 KB）I²C EEPROM
 *
 * 与 AT24C02 相比，引脚和接线完全一样（7 位器件地址同为 0x50），
 * 驱动上有两处不同：
 *   1. 片内地址由 1 字节变为 2 字节（范围 0x0000 ~ 0x7FFF）
 *   2. 页大小由 8 字节变为 64 字节
 * 若跨页连续写入而不拆分，地址会在页内回卷，数据被覆盖。
 * ============================================================================ */

#define AT24C256_SIZE        32768U   /* 32 KB */
#define AT24C256_PAGE_SIZE   64U      /* 每页 64 字节 */
#define AT24C256_WRITE_MS    5U       /* 内部写周期最长 5 ms */

void    bsp_at24c256_init(void);

/* 读一个字节，返回 1 表示器件应答正常，用于上电自检 */
uint8_t bsp_at24c256_probe(void);

/* 读/写任意长度；写函数内部会按页边界自动拆分并等待写周期。
 * 返回 1 成功，0 失败（越界或器件 NACK）。 */
uint8_t bsp_at24c256_read(uint16_t addr, uint8_t *buf, uint16_t len);
uint8_t bsp_at24c256_write(uint16_t addr, const uint8_t *buf, uint16_t len);

#endif /* BSP_AT24C256_H */
