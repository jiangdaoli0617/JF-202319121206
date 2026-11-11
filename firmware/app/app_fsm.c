#include "app_fsm.h"
#include "drv_input.h"
#include "drv_rfid.h"
#include "drv_indicator.h"
#include "drv_display.h"
#include "drv_storage.h"
#include "bsp_usart.h"
#include "board_config.h"
#include "delay.h"

static app_state_t s_state = ST_INIT;

/* ============================================================================
 * 模块 3 + 4 + 6 联合测试：按键 + RC522 读卡 + AT24C256 存储
 *
 * 上电后先对 AT24C256 做一次读写自检（写一组数据再读回比对），
 * 结果常驻 OLED 第 2 行；RC522 版本号自检结果常驻第 1 行。
 * 按键优先：按 K1~K5 任意一个 → OLED 显示键名 + 短鸣。
 * RC522 轮询：没按键时轮询读卡 → 读到卡显示 UID + 短鸣。
 * ============================================================================ */

static const char s_hex[] = "0123456789ABCDEF";
static uint8_t s_rc522_ok = 0U;       /* RC522 自检是否通过 */
static uint8_t s_checked  = 0U;       /* 是否已完成 RC522 自检 */
static uint8_t s_eeprom_ok = 0U;      /* AT24C256 自检是否通过 */
static uint8_t s_eeprom_done = 0U;    /* 是否已完成存储自检 */
static uint16_t s_boot_count = 0U;    /* 上电次数，用于验证掉电保持 */
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

/* 把无符号整数转成十进制字符串（不依赖 printf，节省 ROM） */
static void u16_to_str(char *p, uint16_t v) {
    char    tmp[6];
    uint8_t n = 0U;

    if (v == 0U) {
        p[0] = '0';
        p[1] = '\0';
        return;
    }
    while ((v > 0U) && (n < 5U)) {
        tmp[n++] = (char)('0' + (v % 10U));
        v = (uint16_t)(v / 10U);
    }
    while (n > 0U) {
        *p++ = tmp[--n];
    }
    *p = '\0';
}

/* 显示待机画面 */
static void show_idle(void) {
    char        buf[16];
    char       *q = buf;
    const char *pre = "Boot #";

    drv_display_clear();
    drv_display_line(0U, "Lab Manager");
    drv_display_line(1U, s_rc522_ok ? "RC522  OK" : "RC522  ERR");
    drv_display_line(2U, s_eeprom_ok ? "EEPROM OK" : "EEPROM ERR");

    /* 第 4 行显示上电次数：断电重启后数字递增即说明掉电保存成功 */
    while (*pre != '\0') {
        *q++ = *pre++;
    }
    u16_to_str(q, s_boot_count);
    drv_display_line(3U, buf);

    drv_display_refresh();
}

/* 上电对 AT24C256 做一次读写自检，只跑一次 */
static void eeprom_check_once(void) {
    if (s_eeprom_done) {
        return;
    }
    s_eeprom_done = 1U;

    s_eeprom_ok = drv_storage_selftest();

    if (s_eeprom_ok) {
        s_boot_count = drv_storage_boot_count();
        bsp_usart_send_str(USART_PORT_DEBUG, "[EEPROM] AT24C256 OK\r\n");
    } else {
        bsp_usart_send_str(USART_PORT_DEBUG, "[EEPROM] AT24C256 FAIL\r\n");
    }
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
        /* 0x91/0x92 = 原装 NXP MFRC522；0x82 = 复旦微 FM17522（寄存器兼容） */
        s_rc522_ok = (ver == 0x91U || ver == 0x92U || ver == 0x82U) ? 1U : 0U;
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

    /* 存储自检放在这里：此时 I2C 总线与 OLED 都已初始化完毕 */
    eeprom_check_once();
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
