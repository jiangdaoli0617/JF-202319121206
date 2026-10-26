#include "app_log.h"

static log_entry_t s_logs[STORAGE_MAX_LOGS];
static uint16_t    s_head = 0U;
static uint16_t    s_tail = 0U;
static uint16_t    s_count = 0U;
static uint32_t    s_next_seq = 1U;

void app_log_init(void) {
    uint16_t i;

    s_count = drv_storage_load_logs(s_logs, STORAGE_MAX_LOGS);
    if (s_count > STORAGE_MAX_LOGS) {
        s_count = STORAGE_MAX_LOGS;
    }

    s_tail = 0U;
    s_head = (uint16_t)(s_count % STORAGE_MAX_LOGS);
    s_next_seq = 1U;

    for (i = 0U; i < s_count; i++) {
        if (s_logs[i].seq >= s_next_seq) {
            s_next_seq = s_logs[i].seq + 1U;
        }
    }
}

uint8_t app_log_push(const log_entry_t *e) {
    log_entry_t entry;

    if (e == 0 || s_count >= STORAGE_MAX_LOGS) {
        return 0U;
    }

    entry = *e;
    entry.seq = s_next_seq++;

    s_logs[s_head] = entry;
    s_head = (uint16_t)((s_head + 1U) % STORAGE_MAX_LOGS);
    s_count++;

    /* 同步落盘（≤15 ms）。存储层实现前返回 0，但 RAM 队列已接受。 */
    (void)drv_storage_append_log(&entry);
    return 1U;
}

void app_log_poll(void) {
    /* TODO: 停等发送 → 3 s 超时 → 最多重试 3 次；
     * 收到 ACK 后推进 s_tail、s_count--，并调用 drv_storage_ack_log()。
     * 断网时保持队列不动，联网后从 s_tail 开始补传。 */
}

uint16_t app_log_pending(void) {
    return s_count;
}
