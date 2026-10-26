#ifndef PROTO_CRC_H
#define PROTO_CRC_H

#include <stdint.h>

/* CRC16-MODBUS：多项式 0xA001（0x8005 反射），初值 0xFFFF */
uint16_t proto_crc16(const uint8_t *data, uint16_t len);

/* CRC8：多项式 0x07，初值 0x00，用于 EEPROM 单条日志 */
uint8_t proto_crc8(const uint8_t *data, uint16_t len);

#endif /* PROTO_CRC_H */
