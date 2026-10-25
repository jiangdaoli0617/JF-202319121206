#include "proto_frame.h"
#include "proto_crc.h"
#include <string.h>

uint16_t proto_frame_encode(const proto_frame_t *f, uint8_t *out, uint16_t max) {
    uint16_t total;
    uint16_t crc;
    uint16_t i;

    if (f == 0 || out == 0 || f->payload_len > PROTO_MAX_PAYLOAD) {
        return 0U;
    }

    total = (uint16_t)(PROTO_HEADER_LEN + f->payload_len + PROTO_CRC_LEN);
    if (max < total) {
        return 0U;
    }

    out[0] = PROTO_HEADER0;
    out[1] = PROTO_HEADER1;
    out[2] = f->version;
    out[3] = f->type;
    out[4] = (uint8_t)(f->seq >> 8);
    out[5] = (uint8_t)(f->seq & 0xFFU);
    out[6] = (uint8_t)(f->payload_len >> 8);
    out[7] = (uint8_t)(f->payload_len & 0xFFU);

    for (i = 0U; i < f->payload_len; i++) {
        out[PROTO_HEADER_LEN + i] = f->payload[i];
    }

    /* CRC 覆盖偏移 2 ~ 8+N-1，不含帧头 */
    crc = proto_crc16(&out[2], (uint16_t)(6U + f->payload_len));
    out[8U + f->payload_len]      = (uint8_t)(crc >> 8);
    out[9U + f->payload_len]      = (uint8_t)(crc & 0xFFU);

    return total;
}

uint8_t proto_frame_decode(const uint8_t *in, uint16_t len, proto_frame_t *out) {
    uint16_t payload_len;
    uint16_t crc_calc;
    uint16_t crc_recv;

    if (in == 0 || out == 0 || len < PROTO_FRAME_MIN_LEN) {
        return 0U;
    }
    if (in[0] != PROTO_HEADER0 || in[1] != PROTO_HEADER1) {
        return 0U;
    }

    payload_len = (uint16_t)(((uint16_t)in[6] << 8) | in[7]);
    if (payload_len > PROTO_MAX_PAYLOAD) {
        return 0U;
    }
    if ((uint32_t)PROTO_HEADER_LEN + payload_len + PROTO_CRC_LEN != len) {
        return 0U;
    }

    crc_calc = proto_crc16(&in[2], (uint16_t)(6U + payload_len));
    crc_recv = (uint16_t)(((uint16_t)in[8U + payload_len] << 8) |
                          in[9U + payload_len]);
    if (crc_calc != crc_recv) {
        return 0U;
    }

    out->version     = in[2];
    out->type        = in[3];
    out->seq         = (uint16_t)(((uint16_t)in[4] << 8) | in[5]);
    out->payload_len = payload_len;
    if (payload_len > 0U) {
        memcpy(out->payload, &in[PROTO_HEADER_LEN], payload_len);
    }
    return 1U;
}

void proto_on_frame_received(const uint8_t *frame, uint16_t len) {
    (void)frame;
    (void)len;
    /* TODO: 按 frame->type 分发到 app 层 */
}
