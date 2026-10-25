#ifndef PROTO_TYPES_H
#define PROTO_TYPES_H

/* 与 docs/02-系统架构与通信协议.md 第 4、5 节严格对应。
 * 新增 Tag 时只加宏，旧解析逻辑无需改动。 */

/* ---------- TLV Tag ---------- */
#define TAG_DEVICE_ID      0x01
#define TAG_TIMESTAMP      0x02
#define TAG_EQUIP_ID       0x03
#define TAG_USER_CARD      0x04
#define TAG_OP_TYPE        0x05
#define TAG_RESULT         0x06
#define TAG_STOCK_COUNT    0x07
#define TAG_EQUIP_NAME     0x08
#define TAG_LOG_SEQ        0x09
#define TAG_CARD_UID       0x0A
#define TAG_EQUIP_STATUS   0x0B
#define TAG_USER_NAME      0x0C
#define TAG_DUE_TIME       0x0D
#define TAG_ERROR_MSG      0x0E
#define TAG_TOTAL          0x0F
#define TAG_INDEX          0x10
#define TAG_RECORD_COUNT   0x11
#define TAG_USER_ROLE      0x12

/* ---------- 消息类型 ---------- */
#define MSG_HEARTBEAT      0x01   /* ↑ */
#define MSG_LOG_REPORT     0x02   /* ↑ */
#define MSG_ARCHIVE_REQ    0x03   /* ↑ */
#define MSG_ARCHIVE_CHANGE 0x04   /* ↑ */
#define MSG_BOOT_NOTIFY    0x05   /* ↑ */
#define MSG_USER_REQ       0x06   /* ↑ */
#define MSG_TIME_SYNC      0x10   /* ↓ */
#define MSG_ARCHIVE_DATA   0x11   /* ↓ */
#define MSG_ARCHIVE_ACK    0x12   /* ↓ */
#define MSG_USER_DATA      0x13   /* ↓ */
#define MSG_ACK            0x80   /* ↓ */
#define MSG_ACK_RESP       0x81   /* ↑ */
#define MSG_ERROR          0xFF

/* ---------- OP_TYPE ---------- */
#define OP_LEND            0x01
#define OP_RETURN          0x02
#define OP_REGISTER        0x03
#define OP_DELETE          0x04
#define OP_MANUAL          0x05
#define OP_SYSTEM          0x06
#define OP_EXCEPTION       0x07
#define OP_OVERDUE         0x08

/* ---------- RESULT ---------- */
#define RES_OK             0x00
#define RES_UNAUTHORIZED   0x01
#define RES_UNREGISTERED   0x02
#define RES_ALREADY_LENT   0x03
#define RES_NOT_LENT       0x04
#define RES_TIMEOUT        0x05
#define RES_DUPLICATE      0x06
#define RES_INTERNAL       0x07

/* ---------- USER_ROLE ---------- */
#define ROLE_ADMIN         0x01
#define ROLE_STUDENT       0x02

#endif /* PROTO_TYPES_H */
