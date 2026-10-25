#include "app_inventory.h"
#include "app_config.h"
#include "app_log.h"
#include "proto_types.h"
#include "board_config.h"
#include <string.h>

static equip_record_t s_equip[STORAGE_MAX_EQUIP];
static uint16_t       s_count = 0U;
static uint8_t        s_overdue_logged[STORAGE_MAX_EQUIP];
static uint32_t       s_last_scan = 0U;
static uint8_t        s_scan_started = 0U;

static int16_t find_index(uint16_t equip_id) {
    uint16_t i;

    for (i = 0U; i < s_count; i++) {
        if (s_equip[i].equip_id == equip_id) {
            return (int16_t)i;
        }
    }
    return -1;
}

void app_inventory_init(void) {
    s_count = drv_storage_load_equipment(s_equip, STORAGE_MAX_EQUIP);
    if (s_count > STORAGE_MAX_EQUIP) {
        s_count = STORAGE_MAX_EQUIP;
    }
    memset(s_overdue_logged, 0, sizeof(s_overdue_logged));
    s_scan_started = 0U;
    s_last_scan = 0U;
}

uint16_t app_inventory_bulk_load(const equip_record_t *list, uint16_t count) {
    if (list == 0 && count > 0U) {
        return 0U;
    }
    if (count > STORAGE_MAX_EQUIP) {
        count = STORAGE_MAX_EQUIP;
    }
    if (count > 0U) {
        memcpy(s_equip, list, (size_t)count * sizeof(equip_record_t));
    }
    s_count = count;
    memset(s_overdue_logged, 0, sizeof(s_overdue_logged));

    /* TODO: 按 docs/07 第 7 节先应用服务端档案，再重放本地未确认日志 */
    drv_storage_save_equipment(s_equip, s_count);
    return s_count;
}

uint8_t app_inventory_find(uint16_t equip_id, equip_record_t *out) {
    int16_t idx = find_index(equip_id);

    if (idx < 0) {
        return 0U;
    }
    if (out != 0) {
        *out = s_equip[idx];
    }
    return 1U;
}

uint8_t app_inventory_lend(uint16_t equip_id, uint32_t user_card, uint32_t now) {
    int16_t  idx = find_index(equip_id);
    uint32_t days;

    if (idx < 0 || s_equip[idx].status != EQUIP_IN_STOCK) {
        return 0U;
    }

    days = g_sys_config.overdue_days;
    if (days == 0U) {
        days = 7U;
    }
    if (days > 365U) {
        days = 365U;
    }

    s_equip[idx].status      = EQUIP_LENT;
    s_equip[idx].holder_card = user_card;
    s_equip[idx].lend_time   = now;
    s_equip[idx].due_time    = now + days * 86400UL;
    s_overdue_logged[idx]    = 0U;

    drv_storage_save_equipment(s_equip, s_count);
    return 1U;
}

uint8_t app_inventory_return(uint16_t equip_id, uint32_t user_card, uint32_t now) {
    int16_t idx = find_index(equip_id);

    (void)user_card;   /* 归还人允许与借用人不同，由上层决定是否限制 */
    (void)now;

    if (idx < 0) {
        return 0U;
    }
    if (s_equip[idx].status != EQUIP_LENT &&
        s_equip[idx].status != EQUIP_OVERDUE) {
        return 0U;
    }

    s_equip[idx].status      = EQUIP_IN_STOCK;
    s_equip[idx].holder_card = 0U;
    s_equip[idx].lend_time   = 0U;
    s_equip[idx].due_time    = 0U;
    s_overdue_logged[idx]    = 0U;

    drv_storage_save_equipment(s_equip, s_count);
    return 1U;
}

uint16_t app_inventory_count_in_stock(void) {
    uint16_t i;
    uint16_t n = 0U;

    for (i = 0U; i < s_count; i++) {
        if (s_equip[i].status == EQUIP_IN_STOCK) {
            n++;
        }
    }
    return n;
}

void app_inventory_poll(uint32_t now) {
    uint16_t i;
    uint8_t  dirty = 0U;

    if (!s_scan_started) {
        s_scan_started = 1U;
        s_last_scan = now;
    } else {
        if ((uint32_t)(now - s_last_scan) < OVERDUE_SCAN_SEC) {
            return;
        }
        s_last_scan = now;
    }

    for (i = 0U; i < s_count; i++) {
        if (s_equip[i].status == EQUIP_LENT &&
            s_equip[i].due_time != 0U &&
            now >= s_equip[i].due_time) {

            s_equip[i].status = EQUIP_OVERDUE;
            dirty = 1U;

            if (!s_overdue_logged[i]) {
                log_entry_t e;

                memset(&e, 0, sizeof(e));
                e.op_type   = OP_OVERDUE;
                e.result    = RES_OK;
                e.equip_id  = s_equip[i].equip_id;
                e.user_card = s_equip[i].holder_card;
                e.timestamp = now;
                e.due_time  = s_equip[i].due_time;

                if (app_log_push(&e)) {
                    s_overdue_logged[i] = 1U;
                }
            }
        }
    }

    if (dirty) {
        drv_storage_save_equipment(s_equip, s_count);
    }
}

uint16_t app_inventory_count_overdue(void) {
    uint16_t i;
    uint16_t n = 0U;

    for (i = 0U; i < s_count; i++) {
        if (s_equip[i].status == EQUIP_OVERDUE) {
            n++;
        }
    }
    return n;
}

uint16_t app_inventory_get_overdue_list(equip_record_t *out, uint16_t max) {
    uint16_t i;
    uint16_t n = 0U;

    if (out == 0) {
        return 0U;
    }
    for (i = 0U; i < s_count && n < max; i++) {
        if (s_equip[i].status == EQUIP_OVERDUE) {
            out[n++] = s_equip[i];
        }
    }
    return n;
}
