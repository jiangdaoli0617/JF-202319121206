#ifndef DRV_STORAGE_H
#define DRV_STORAGE_H

#include <stdint.h>

/* 容量由 AT24C02 的 2KB 预算决定，见 docs/07-EEPROM存储设计.md。
 * 新增持久化字段前必须先核算预算，不能原地追加。 */
#define STORAGE_MAX_USERS   40
#define STORAGE_MAX_EQUIP   22
#define STORAGE_MAX_LOGS    36

/* 与 docs/02 4.4 节一致；status 字段为 uint8_t，此处仅作可读常量 */
typedef enum {
    EQUIP_IN_STOCK = 0x00,
    EQUIP_LENT     = 0x01,
    EQUIP_OVERDUE  = 0x02,
    EQUIP_UNKNOWN  = 0x03,
    EQUIP_DISABLED = 0x04
} equip_status_t;

typedef struct {
    uint32_t card;                /* UID 低 32 位 */
    uint8_t  role;                /* 1=管理员，2=学生 */
} user_card_t;

typedef struct {
    uint16_t equip_id;
    uint8_t  status;
    uint32_t holder_card;         /* 在库为 0 */
    uint32_t lend_time;           /* Unix 秒 */
    uint32_t due_time;            /* Unix 秒 */
    char     name[16];            /* 仅 RAM；不持久化 */
} equip_record_t;

typedef struct {
    uint32_t seq;                 /* 与协议 LOG_SEQ 一致 */
    uint8_t  op_type;
    uint8_t  result;
    uint16_t equip_id;
    uint32_t user_card;
    uint32_t timestamp;
    uint32_t due_time;
} log_entry_t;

typedef struct {
    char     device_id[12];
    char     wifi_ssid[24];
    char     wifi_pass[24];
    char     server_host[16];
    uint16_t server_port;
    uint16_t overdue_days;
} sys_config_t;

void     drv_storage_init(void);

/* 以下 load 自动选用 A/B 中校验通过且 seq 较大的一份 */
uint8_t  drv_storage_load_config(sys_config_t *cfg);
uint16_t drv_storage_load_users(user_card_t *out, uint16_t max);
uint16_t drv_storage_load_equipment(equip_record_t *out, uint16_t max);
uint16_t drv_storage_load_logs(log_entry_t *out, uint16_t max);

/* 以下 save 只置脏并返回，由 drv_storage_poll 分页写入 */
void     drv_storage_save_config(const sys_config_t *cfg);
uint8_t  drv_storage_save_users(const user_card_t *list, uint16_t count);
uint8_t  drv_storage_save_equipment(const equip_record_t *list, uint16_t count);

/* 追加日志并同步落盘（≤15 ms）；返回 0 表示队列已满 */
uint8_t  drv_storage_append_log(const log_entry_t *e);
void     drv_storage_ack_log(void);
void     drv_storage_poll(void);
uint8_t  drv_storage_flush(void);

#endif /* DRV_STORAGE_H */
