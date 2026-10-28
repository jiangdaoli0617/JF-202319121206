#ifndef BSP_RC522_H
#define BSP_RC522_H

#include <stdint.h>

void    bsp_rc522_init(void);
/* 读取 VersionReg(0x37)：正常应为 0x91 或 0x92，用于验证 SPI 通路。
 * 必须在 bsp_rc522_init() 之后读，复位完成前读到的值不可信。 */
uint8_t bsp_rc522_version(void);
/* 读一张卡的 UID：内部完成 寻卡 → 防冲突 → 选卡 → 挂起 四步。
 * 读到卡返回 1 并把 4 字节 UID 写进 uid；无卡或出错返回 0。
 * 全过程有超时保护（最长约 20ms），可以在主循环里直接轮询调用。 */
uint8_t bsp_rc522_read_uid(uint32_t *uid);
void    bsp_rc522_antenna(uint8_t on);

#endif /* BSP_RC522_H */
