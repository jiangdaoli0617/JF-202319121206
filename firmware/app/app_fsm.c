#include "app_fsm.h"
#include "drv_rfid.h"
#include "drv_indicator.h"
#include "drv_display.h"

static app_state_t s_state = ST_INIT;

/* ============================================================================
 * 模块 6 硬件测试：RC522 读卡
 *
 * 上电先读一次 VersionReg（正常 0x91/0x92）验证 SPI 通路：
 *   - 读得到 → OLED 显示 RC522 OK，进入读卡轮询；
 *   - 读不到 → OLED 显示 RC522 ERR，提示检查 SPI 接线，不进入读卡。
 * 读到卡片后把 4 字节 UID 显示到 OLED 并短鸣一声，换另一张卡会刷新。
 *
 * 真正的双卡流程（先身份卡、再器材卡）要等状态机实现后再替换这里。
 * ============================================================================ */

static const char s_hex[] = "0123456789ABCDEF";
static uint8_t s_checked = 0U;   /* 是否已完成上电自检 */
static uint8_t s_ver_ok  = 0U;   /* 自检结果：1 = SPI 通路正常 */
static uint8_t s_ver_val = 0U;   /* 自检读到的 VersionReg 原始值，用于诊断 */

/* 把 4 字节 UID 转成 8 位大写十六进制串，如 A1B2C3D4 */
static void uid_hex_str(char *buf, uint32_t uid) {
    uint8_t i;

    for (i = 0U; i < 8U; i++) {
        buf[7U - i] = s_hex[uid & 0x0FU];
        uid >>= 4U;
    }
    buf[8] = '\0';
}

/* 显示待刷卡画面（含 SPI 自检结果；失败时把版本号原始值显示出来帮助定位） */
static void rc522_show_idle(void) {
    char ver[7];

    drv_display_clear();
    if (s_ver_ok) {
        drv_display_line(0U, "RC522 OK");
        drv_display_line(1U, "Swipe card");
    } else {
        ver[0] = 'V'; ver[1] = 'E'; ver[2] = 'R'; ver[3] = '=';
        ver[4] = s_hex[s_ver_val >> 4];
        ver[5] = s_hex[s_ver_val & 0x0FU];
        ver[6] = '\0';
        drv_display_line(0U, "RC522 ERR");
        drv_display_line(1U, ver);
    }
    drv_display_refresh();
}

static void rc522_test(void) {
    rfid_card_t card;
    char uid[9];

    /* 第一次进来先自检一次 SPI 通路 */
    if (!s_checked) {
        s_ver_val = drv_rfid_version();
        s_ver_ok  = (s_ver_val == 0x91U || s_ver_val == 0x92U) ? 1U : 0U;
        s_checked = 1U;
        rc522_show_idle();
    }

    if (!s_ver_ok) {
        return;                     /* SPI 没通，先别轮询读卡 */
    }

    if (drv_rfid_poll(&card)) {
        uid_hex_str(uid, card.uid);
        drv_indicator_show(IND_KEY);
        drv_display_clear();
        drv_display_line(0U, "RC522 OK");
        drv_display_line(1U, "Card UID:");
        drv_display_line(2U, uid);
        drv_display_refresh();
    }
}

void app_fsm_init(void) {
    s_state = ST_IDLE;
}

void app_fsm_run(void) {
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
