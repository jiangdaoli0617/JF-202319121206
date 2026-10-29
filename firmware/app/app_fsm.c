#include "app_fsm.h"
#include "drv_input.h"
#include "drv_indicator.h"
#include "app_ui.h"

static app_state_t s_state = ST_INIT;

void app_fsm_init(void) {
    s_state = ST_IDLE;
}

/* 模块 3 硬件测试：按键 → OLED 显示键名 + 蜂鸣一声。
 * 真正的登记/借出/归还流程要等双卡状态机实现后再替换这里。 */
static void key_test(void) {
    key_id_t key = drv_input_scan();

    if (key == KEY_NONE) {
        return;
    }

    drv_indicator_show(IND_KEY);

    switch (key) {
        case KEY_K1: app_ui_show_prompt("K1", "REGISTER"); break;
        case KEY_K2: app_ui_show_prompt("K2", "LEND");     break;
        case KEY_K3: app_ui_show_prompt("K3", "RETURN");   break;
        case KEY_K4: app_ui_show_prompt("K4", "PREV");     break;
        case KEY_K5: app_ui_show_prompt("K5", "NEXT");     break;
        default: break;
    }
}

void app_fsm_run(void) {
    key_test();

    /* TODO: 双卡流程
     *   ST_IDLE     → drv_rfid_poll → app_auth_check
     *   ST_AUTH_OK  → 15 s 内再刷器材卡，否则回 ST_IDLE
     *   ST_EXECUTE  → app_inventory_lend/return → app_log_push → flush → 提示
     *   ST_ADMIN_MENU → 按键翻页查看超期列表
     * 所有超时均用 app_fsm 内部时间戳判断，禁止 delay。 */
}

app_state_t app_fsm_state(void) {
    return s_state;
}
