#ifndef PROTO_TLV_H
#define PROTO_TLV_H

#include <stdint.h>

/* 载荷格式：Tag(1) + Len(1) + Value(Len)，顺序拼接。
 * 接收方遇到未知 Tag 时因 Len 已知可直接跳过，因此新增字段不影响旧解析。 */

#define PROTO_TLV_HEADER_LEN 2U

/* 编码一个 TLV。返回写入字节数，空间不足返回 0 */
uint16_t proto_tlv_encode(uint8_t *buf, uint16_t max,
                          uint8_t tag, const uint8_t *value, uint8_t len);

/* 在载荷中查找 Tag。找到返回 1，并输出 value 指针与长度 */
uint8_t proto_tlv_find(const uint8_t *payload, uint16_t len, uint8_t tag,
                       const uint8_t **value, uint8_t *value_len);

/* 常用类型快捷编码，数值一律大端 */
uint16_t proto_tlv_put_u8(uint8_t *buf, uint16_t max, uint8_t tag, uint8_t v);
uint16_t proto_tlv_put_u16(uint8_t *buf, uint16_t max, uint8_t tag, uint16_t v);
uint16_t proto_tlv_put_u32(uint8_t *buf, uint16_t max, uint8_t tag, uint32_t v);
uint16_t proto_tlv_put_str(uint8_t *buf, uint16_t max, uint8_t tag, const char *s);

#endif /* PROTO_TLV_H */
