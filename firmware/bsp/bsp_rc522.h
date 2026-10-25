#ifndef BSP_RC522_H
#define BSP_RC522_H

#include <stdint.h>

void    bsp_rc522_init(void);
/* 读取 VersionReg(0x37)：正常应为 0x91 或 0x92，用于验证 SPI 通路 */
uint8_t bsp_rc522_version(void);
/* 非阻塞读卡；读到返回 1 并写 uid，无卡返回 0。
 * 完整寻卡/防冲突流程待实现。 */
uint8_t bsp_rc522_read_uid(uint32_t *uid);
void    bsp_rc522_antenna(uint8_t on);

#endif /* BSP_RC522_H */
