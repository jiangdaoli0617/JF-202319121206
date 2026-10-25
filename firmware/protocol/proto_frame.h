#ifndef PROTO_FRAME_H
#define PROTO_FRAME_H

#include <stdint.h>

#define PROTO_HEADER0        0xAAU
#define PROTO_HEADER1        0x55U
#define PROTO_VERSION        0x01U
#define PROTO_HEADER_LEN     8U    /* AA 55 + 版本 + 类型 + 序号 + 长度 */
#define PROTO_CRC_LEN        2U
#define PROTO_FRAME_MIN_LEN  (PROTO_HEADER_LEN + PROTO_CRC_LEN)
#define PROTO_MAX_PAYLOAD    128U

typedef struct {
    uint8_t  version;
    uint8_t  type;
    uint16_t seq;
    uint16_t payload_len;
    uint8_t  payload[PROTO_MAX_PAYLOAD];
} proto_frame_t;

/* 编码一帧。返回总字节数，空间不足或参数非法返回 0 */
uint16_t proto_frame_encode(const proto_frame_t *f, uint8_t *out, uint16_t max);
/* 解码一帧（输入必须是完整一帧）。成功返回 1 */
uint8_t  proto_frame_decode(const uint8_t *in, uint16_t len, proto_frame_t *out);

/* 网络层收到的完整帧入口，由 drv_net 回调。当前为业务分发桩 */
void     proto_on_frame_received(const uint8_t *frame, uint16_t len);

#endif /* PROTO_FRAME_H */
