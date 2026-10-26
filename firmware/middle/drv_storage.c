#include "drv_storage.h"
#include "bsp_at24c02.h"

/* TODO: 按 docs/07 实现固定分区、A/B 双备份、环形日志队列。
 * 建议实现顺序：
 *   1. 配置块读写 + A/B 选择
 *   2. 卡表块
 *   3. 档案块
 *   4. 日志环形队列与上电恢复
 *   5. 分页写入与 flush
 * 在此之前，所有 load 返回 0，上层按「无有效数据」处理。 */

void drv_storage_init(void) {
    bsp_at24c02_init();
}

uint8_t drv_storage_load_config(sys_config_t *cfg) {
    (void)cfg;
    return 0U;
}

uint16_t drv_storage_load_users(user_card_t *out, uint16_t max) {
    (void)out;
    (void)max;
    return 0U;
}

uint16_t drv_storage_load_equipment(equip_record_t *out, uint16_t max) {
    (void)out;
    (void)max;
    return 0U;
}

uint16_t drv_storage_load_logs(log_entry_t *out, uint16_t max) {
    (void)out;
    (void)max;
    return 0U;
}

void drv_storage_save_config(const sys_config_t *cfg) {
    (void)cfg;
}

uint8_t drv_storage_save_users(const user_card_t *list, uint16_t count) {
    (void)list;
    (void)count;
    return 0U;
}

uint8_t drv_storage_save_equipment(const equip_record_t *list, uint16_t count) {
    (void)list;
    (void)count;
    return 0U;
}

uint8_t drv_storage_append_log(const log_entry_t *e) {
    (void)e;
    return 0U;
}

void drv_storage_ack_log(void) {
}

void drv_storage_poll(void) {
}

uint8_t drv_storage_flush(void) {
    return 1U;
}
