#ifndef BSP_AT24C02_H
#define BSP_AT24C02_H

#include <stdint.h>

/* AT24C02 容量 256 B，地址 0x00~0xFF。
 * addr 用 uint16_t 是为将来换 AT24C32/64 预留，上层接口无需改动。 */
#define AT24C02_SIZE  256U

void    bsp_at24c02_init(void);
uint8_t bsp_at24c02_read(uint16_t addr, uint8_t *buf, uint16_t len);
uint8_t bsp_at24c02_write(uint16_t addr, const uint8_t *buf, uint16_t len);

#endif /* BSP_AT24C02_H */
