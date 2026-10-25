#ifndef APP_AUTH_H
#define APP_AUTH_H

#include "drv_storage.h"

typedef enum {
    AUTH_NONE = 0,
    AUTH_STUDENT,
    AUTH_ADMIN
} auth_role_t;

void        app_auth_init(void);        /* 从 EEPROM 加载卡表 */
auth_role_t app_auth_check(uint32_t card_uid);
/* 收到 USER_DATA 后整体替换；返回 1 表示已接受 */
uint8_t     app_auth_replace(const user_card_t *list, uint16_t count);
uint16_t    app_auth_count(void);

#endif /* APP_AUTH_H */
