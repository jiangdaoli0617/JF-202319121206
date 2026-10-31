#include "app_fsm.h"
#include "drv_input.h"
#include "drv_rfid.h"
#include "drv_indicator.h"
#include "drv_display.h"
#include "bsp_usart.h"
#include "board_config.h"
#include "delay.h"

static app_state_t s_state = ST_INIT;

/* ============================================================================
 * 模块 3 + 模块 6 联合测试：按键 + RC522 读卡
 *
 * 按键优先：按 K1~K5 任意一个 → OLED 显示键名 + 短鸣。
 * RC522 轮询：没按键时轮询读卡 → 读到卡显示 UID + 短鸣。
 *
 * 上电自检 RC522 版本号，如果 SPI 不通（VER != 0x91/0x92）就不轮询读卡，
 * 避免 SPI 卡死影响按键。
 * ============================================================================ */

static const char s_hex[] = "0123456789ABCDEF";
static uint8_t s_rc522_ok = 0U;       /* RC522 自检是否通过 */
static uint8_t s_checked  = 0U;       /* 是否已完成上电自检 */
static uint32_t s_last_display_ms = 0U;   /* 上次刷新 OLED 的时间 */

/* 把 4 字节 UID 转成 8 位大写十六进制串 */
static void uid_hex_str(char *buf, uint32_t uid) {
    uint8_t i;
    for (i = 0U; i < 8U; i++) {
        buf[7U - i] = s_hex[uid & 0x0FU];
        uid >>= 4U;
    }
    buf[8] = '\0';
}

/* 显示待机画面 */
static void show_idle(void) {
    drv_display_clear();
    drv_display_line(0U, "Lab Manager");
    if (s_rc522_ok) {
        drv_display_line(2U, "Swipe / Press");
    } else {
        drv_display_line(2U, "Press key");
        drv_display_line(3U, "RC522 offline");
    }
    drv_display_refresh();
}

/* 按键测试（优先级高于读卡） */
static uint8_t key_test(void) {
    key_id_t key = drv_input_scan();

    if (key == KEY_NONE) {
        return 0U;
    }

    drv_indicator_show(IND_KEY);
    drv_display_clear();

    switch (key) {
        case KEY_K1: drv_display_line(1U, "K1"); drv_display_line(3U, "REGISTER"); break;
        case KEY_K2: drv_display_line(1U, "K2"); drv_display_line(3U, "LEND");     break;
        case KEY_K3: drv_display_line(1U, "K3"); drv_display_line(3U, "RETURN");   break;
        case KEY_K4: drv_display_line(1U, "K4"); drv_display_line(3U, "PREV");     break;
        case KEY_K5: drv_display_line(1U, "K5"); drv_display_line(3U, "NEXT");     break;
        default: break;
    }

    drv_display_refresh();
    s_last_display_ms = g_sys_tick_ms;
    return 1U;
}

/* RC522 读卡测试（按键没按时才轮询） */
static void rc522_test(void) {
    rfid_card_t card;
    char uid[9];

    /* 第一次进来先自检 SPI 通路 */
    if (!s_checked) {
        uint8_t ver = drv_rfid_version();
        s_rc522_ok = (ver == 0x91U || ver == 0x92U) ? 1U : 0U;
        s_checked  = 1U;

        if (s_rc522_ok) {
            /* 手动重新打开天线 */
            drv_rfid_antenna(0);
            delay_ms(10);
            drv_rfid_antenna(1);
            bsp_usart_send_str(USART_PORT_DEBUG, "[RC522] Antenna ON\r\n");
        } else {
            bsp_usart_send_str(USART_PORT_DEBUG, "[RC522] SPI check failed, VER=0x");
            char hex[3];
            hex[0] = s_hex[ver >> 4];
            hex[1] = s_hex[ver & 0x0FU];
            hex[2] = '\0';
            bsp_usart_send_str(USART_PORT_DEBUG, hex);
            bsp_usart_send_str(USART_PORT_DEBUG, "\r\n");
        }

        show_idle();
        s_last_display_ms = g_sys_tick_ms;
        return;
    }

    /* SPI 不通就不轮询读卡，避免卡死 */
    if (!s_rc522_ok) {
        return;
    }

    /* 距离上次刷新 OLED < 2 秒，说明刚按过键或刚读过卡，先不刷新待机画面 */
    if ((uint32_t)(g_sys_tick_ms - s_last_display_ms) < 2000U) {
        /* 但仍然轮询读卡，读到了就更新画面 */
        if (drv_rfid_poll(&card)) {
            uid_hex_str(uid, card.uid);
            drv_indicator_show(IND_KEY);

            bsp_usart_send_str(USART_PORT_DEBUG, "[RFID] UID=");
            bsp_usart_send_str(USART_PORT_DEBUG, uid);
            bsp_usart_send_str(USART_PORT_DEBUG, "\r\n");

            drv_display_clear();
            drv_display_line(0U, "Card UID:");
            drv_display_line(2U, uid);
            drv_display_refresh();
            s_last_display_ms = g_sys_tick_ms;
        }
        return;
    }

    /* 超过 2 秒没动作，恢复待机画面 */
    if ((uint32_t)(g_sys_tick_ms - s_last_display_ms) == 2000U) {
        show_idle();
        s_last_display_ms = g_sys_tick_ms + 1U;  /* +1 避免每个 tick 都重画 */
    }

    /* 继续轮询读卡 */
    if (drv_rfid_poll(&card)) {
        uid_hex_str(uid, card.uid);
        drv_indicator_show(IND_KEY);

        bsp_usart_send_str(USART_PORT_DEBUG, "[RFID] UID=");
        bsp_usart_send_str(USART_PORT_DEBUG, uid);
        bsp_usart_send_str(USART_PORT_DEBUG, "\r\n");

        drv_display_clear();
        drv_display_line(0U, "Card UID:");
        drv_display_line(2U, uid);
        drv_display_refresh();
        s_last_display_ms = g_sys_tick_ms;
    }
}

void app_fsm_init(void) {
    s_state = ST_IDLE;
    s_last_display_ms = 0U;
}

void app_fsm_run(void) {
    /* 按键优先级高于读卡 */
    if (key_test()) {
        return;
    }

    /* 没按键再轮询读卡 */
    rc522_test();

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
