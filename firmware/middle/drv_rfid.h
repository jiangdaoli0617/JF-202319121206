#ifndef DRV_RFID_H
#define DRV_RFID_H

#include <stdint.h>

typedef struct {
    uint32_t uid;          /* 4 字节 UID 压缩为 uint32，取低 32 位 */
    uint8_t  uid_len;      /* Mifare S50 为 4 */
    uint8_t  valid;        /* 1 = 读到有效卡 */
} rfid_card_t;

void    drv_rfid_init(void);
/* 读 VersionReg 原始值，正常为 0x91/0x92，用于验证 SPI 通路 */
uint8_t drv_rfid_version(void);
/* 非阻塞轮询，无卡返回 0。内部含 2 s 同卡去重 */
uint8_t drv_rfid_poll(rfid_card_t *card);
void    drv_rfid_antenna(uint8_t on);

#endif /* DRV_RFID_H */
