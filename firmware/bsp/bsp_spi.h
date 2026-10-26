#ifndef BSP_SPI_H
#define BSP_SPI_H

#include <stdint.h>

void    bsp_spi_init(void);
/* 全双工收发一个字节；超时返回 0xFF */
uint8_t bsp_spi_transfer(uint8_t data);
/* level: 1 = 拉高（不选中），0 = 拉低（选中） */
void    bsp_spi_cs(uint8_t level);

#endif /* BSP_SPI_H */
