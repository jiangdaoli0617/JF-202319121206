#ifndef BSP_I2C_H
#define BSP_I2C_H

#include <stdint.h>

void    bsp_i2c_init(void);

/* 底层是 GPIO 软件模拟时序（SCL=PB6，SDA=PB7），不使用硬件 I2C 外设。
 * 以下函数返回 1 成功，0 失败（NACK）。
 * addr 为 7 位地址，如 0x50、0x68、0x3C。 */
uint8_t bsp_i2c_write(uint8_t addr, const uint8_t *data, uint16_t len);
uint8_t bsp_i2c_read(uint8_t addr, uint8_t *data, uint16_t len);
uint8_t bsp_i2c_write_reg(uint8_t addr, uint8_t reg,
                          const uint8_t *data, uint16_t len);
uint8_t bsp_i2c_read_reg(uint8_t addr, uint8_t reg,
                         uint8_t *data, uint16_t len);

/* 扫描 0x01~0x7F，把在线地址写入 found，返回发现数量 */
uint8_t bsp_i2c_scan(uint8_t *found, uint8_t max);

#endif /* BSP_I2C_H */
