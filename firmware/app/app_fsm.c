#include "app_fsm.h"
#include <string.h>
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
static uint8_t s_stor_ok = 0U;        /* 存储层往返测试是否通过 */
static uint8_t s_eeprom_done = 0U;    /* 是否已完成存储自检 */
static uint16_t s_boot_count = 0U;    /* 上电次数，用于验证掉电保持 */
static uint32_t s_last_display_ms = 0U;   /* 上次刷新 OLED 的时间 */

/* 定长字符串写入，不足补 '\0' */
static void str_set(char *dst, uint16_t size, const char *src) {
    uint16_t i = 0U;

    while ((uint16_t)(i + 1U) < size && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

/* ============================================================================
 * 存储层往返测试（仅用于模块 4 上机验证）
 *
 * 依次验证四类数据的「写入 → 读回 → 逐字段比对」：
 *   1. 配置：连续保存两次，应读到第二次写入的值（验证 A/B 副本切换）
 *   2. 人员卡表：3 张卡的卡号与角色
 *   3. 器材表：2 件器材的状态与时间戳
 *   4. 日志：追加 2 条后按从旧到新的顺序读回
 * ============================================================================ */
static uint8_t storage_roundtrip_test(void) {
    sys_config_t   cw, cr;
    user_card_t    uw[3], ur[3];
    equip_record_t ew[2], er[2];
    log_entry_t    lw;
    log_entry_t    lr[4];
    uint16_t       n;
    uint8_t        i;

    /* ---- 1. 配置：写两次，必须读到第二次的值 ---- */
    memset(&cw, 0, sizeof(cw));
    str_set(cw.device_id,  (uint16_t)sizeof(cw.device_id),  "LAB-01-A");
    str_set(cw.wifi_ssid,  (uint16_t)sizeof(cw.wifi_ssid),  "test-wifi");
    str_set(cw.wifi_pass,  (uint16_t)sizeof(cw.wifi_pass),  "12345678");
    str_set(cw.server_host,(uint16_t)sizeof(cw.server_host),"192.168.1.100");
    cw.server_port  = 9000U;
    cw.overdue_days = 7U;
    drv_storage_save_config(&cw);

    str_set(cw.device_id,  (uint16_t)sizeof(cw.device_id),  "LAB-02-B");
    cw.server_port  = 9100U;
    cw.overdue_days = 3U;
    drv_storage_save_config(&cw);

    memset(&cr, 0, sizeof(cr));
    if (!drv_storage_load_config(&cr)) {
        return 0U;
    }
    if (strcmp(cr.device_id, "LAB-02-B") != 0) {
        return 0U;                       /* A/B 切换失败 */
    }
    if ((cr.server_port != 9100U) || (cr.overdue_days != 3U)) {
        return 0U;
    }

    /* ---- 2. 人员卡表 ---- */
    uw[0].card = 0x03FA843EUL; uw[0].role = 1U;
    uw[1].card = 0xA1B2C3D4UL; uw[1].role = 2U;
    uw[2].card = 0x11223344UL; uw[2].role = 2U;
    if (!drv_storage_save_users(uw, 3U)) {
        return 0U;
    }
    n = drv_storage_load_users(ur, 3U);
    if (n != 3U) {
        return 0U;
    }
    for (i = 0U; i < 3U; i++) {
        if ((ur[i].card != uw[i].card) || (ur[i].role != uw[i].role)) {
            return 0U;
        }
    }

    /* ---- 3. 器材表 ---- */
    ew[0].equip_id = 1U;  ew[0].status = EQUIP_IN_STOCK;
    ew[0].holder_card = 0UL;             ew[0].lend_time = 0UL;
    ew[0].due_time = 0UL;                ew[0].name[0] = '\0';
    ew[1].equip_id = 2U;  ew[1].status = EQUIP_LENT;
    ew[1].holder_card = 0x03FA843EUL;    ew[1].lend_time = 1767225600UL;
    ew[1].due_time = 1767830400UL;       ew[1].name[0] = '\0';
    if (!drv_storage_save_equipment(ew, 2U)) {
        return 0U;
    }
    n = drv_storage_load_equipment(er, 2U);
    if (n != 2U) {
        return 0U;
    }
    for (i = 0U; i < 2U; i++) {
        if ((er[i].equip_id != ew[i].equip_id) ||
            (er[i].status != ew[i].status) ||
            (er[i].holder_card != ew[i].holder_card) ||
            (er[i].lend_time != ew[i].lend_time) ||
            (er[i].due_time != ew[i].due_time)) {
            return 0U;
        }
    }

    /* ---- 4. 日志：追加两条，读回时应按从旧到新排列 ---- */
    lw.seq = 1001UL; lw.op_type = 1U; lw.result = 0U; lw.equip_id = 2U;
    lw.user_card = 0x03FA843EUL; lw.timestamp = 1767225600UL;
    lw.due_time = 1767830400UL;
    if (!drv_storage_append_log(&lw)) {
        return 0U;
    }

    lw.seq = 1002UL; lw.op_type = 2U; lw.result = 0U; lw.equip_id = 2U;
    lw.user_card = 0x03FA843EUL; lw.timestamp = 1767225700UL;
    lw.due_time = 0UL;
    if (!drv_storage_append_log(&lw)) {
        return 0U;
    }

    n = drv_storage_load_logs(lr, 4U);
    if (n < 2U) {
        return 0U;
    }
    if ((lr[n - 2U].seq != 1001UL) || (lr[n - 1U].seq != 1002UL)) {
        return 0U;                       /* 顺序不对 */
    }
    if ((lr[n - 2U].user_card != 0x03FA843EUL) ||
        (lr[n - 1U].op_type != 2U)) {
        return 0U;
    }

    return 1U;
}

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
    const char *pre;

    drv_display_clear();
    drv_display_line(0U, "Lab Manager");
    drv_display_line(1U, s_rc522_ok ? "RC522  OK" : "RC522  ERR");
    drv_display_line(2U, s_eeprom_ok ? "EEPROM OK" : "EEPROM ERR");

    /* 第 4 行：存储层测试结果 + 上电次数
     * 断电重启后 # 后面的数字递增，即说明掉电保存成功 */
    pre = s_stor_ok ? "StorOK #" : "StorNG #";
    while (*pre != '\0') {
        *q++ = *pre++;
    }
    u16_to_str(q, s_boot_count);
    drv_display_line(3U, buf);

    drv_display_refresh();
}

/* 上电对 AT24C256 做一次读写自检和存储层往返测试，只跑一次 */
static void eeprom_check_once(void) {
    if (s_eeprom_done) {
        return;
    }
    s_eeprom_done = 1U;

    s_eeprom_ok = drv_storage_selftest();

    if (!s_eeprom_ok) {
        bsp_usart_send_str(USART_PORT_DEBUG, "[EEPROM] AT24C256 FAIL\r\n");
        return;
    }
    bsp_usart_send_str(USART_PORT_DEBUG, "[EEPROM] AT24C256 OK\r\n");

    /* 器件能读写之后，再验证分区 / A/B 备份 / 日志环是否工作正常 */
    s_stor_ok = storage_roundtrip_test();
    bsp_usart_send_str(USART_PORT_DEBUG,
                       s_stor_ok ? "[STOR] roundtrip OK\r\n"
                                 : "[STOR] roundtrip FAIL\r\n");

    s_boot_count = drv_storage_boot_count();
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
