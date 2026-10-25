#include "app_auth.h"
#include "proto_types.h"
#include <string.h>

static user_card_t s_cards[STORAGE_MAX_USERS];
static uint16_t    s_count = 0U;

void app_auth_init(void) {
    s_count = drv_storage_load_users(s_cards, STORAGE_MAX_USERS);
    if (s_count > STORAGE_MAX_USERS) {
        s_count = STORAGE_MAX_USERS;
    }
}

auth_role_t app_auth_check(uint32_t card_uid) {
    uint16_t i;

    for (i = 0U; i < s_count; i++) {
        if (s_cards[i].card == card_uid) {
            return (s_cards[i].role == ROLE_ADMIN) ? AUTH_ADMIN : AUTH_STUDENT;
        }
    }
    return AUTH_NONE;
}

uint8_t app_auth_replace(const user_card_t *list, uint16_t count) {
    if (list == 0 && count > 0U) {
        return 0U;
    }
    if (count > STORAGE_MAX_USERS) {
        count = STORAGE_MAX_USERS;
    }
    if (count > 0U) {
        memcpy(s_cards, list, (size_t)count * sizeof(user_card_t));
    }
    s_count = count;

    /* TODO: 存储层实现后，这里改为「置脏 + 分页写入」；写入失败应回滚并返回 0 */
    (void)drv_storage_save_users(s_cards, s_count);
    return 1U;
}

uint16_t app_auth_count(void) {
    return s_count;
}
