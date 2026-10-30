#include "app_fsm.h"
#include "drv_input.h"
#include "drv_indicator.h"
#include "drv_rtc.h"
#include "drv_display.h"

static app_state_t s_state = ST_INIT;
static uint32_t    s_last_sec = 0U;

/* ============================================================================
 * 模块 5 硬件测试：DS3231 实时时钟
 *
 * 上电后 OLED 第 0 行显示 RTC 状态（OK / LOST），第 1、2 行显示当前日期和
 * 时间，每 1 秒刷新一次。按 K1（登记键）把 DS3231 写成下面的固定时间，用来
 * 验证「写时间 → 走时 → 断电保持」三个环节。
 *
 * LOST 表示 DS3231 的状态寄存器 OSF 位为 1：要么是全新芯片没设置过时间，
 * 要么电池没电/刚断过电。按 K1 设置一次后 OSF 会被清除，重新显示 OK。
 *
 * 真正的登记/借出/归还流程要等双卡状态机实现后再替换这里。
 * ============================================================================ */

/* 验证用初始时间（最终真实校时由上位机下发，不走按键） */
#define RTC_TEST_YEAR    2026
#define RTC_TEST_MONTH   10
#define RTC_TEST_DAY     30
#define RTC_TEST_HOUR    12
#define RTC_TEST_MINUTE  0
#define RTC_TEST_SECOND  0

/* 把 0~99 的两位十进制数写入 p（带前导零），共写 2 个字符 */
static void put_two(char *p, uint8_t v) {
    p[0] = (char)('0' + (v / 10U));
    p[1] = (char)('0' + (v % 10U));
}

/* 生成日期串 "YYYY-MM-DD"，占 10 个字符 */
static void rtc_date_str(char *buf, const rtc_time_t *t) {
    buf[0]  = (char)('0' + ((uint8_t)(t->year / 1000U) % 10U));
    buf[1]  = (char)('0' + ((uint8_t)(t->year / 100U) % 10U));
    buf[2]  = (char)('0' + ((uint8_t)(t->year / 10U) % 10U));
    buf[3]  = (char)('0' + (uint8_t)(t->year % 10U));
    buf[4]  = '-';
    put_two(&buf[5], t->month);
    buf[7]  = '-';
    put_two(&buf[8], t->day);
    buf[10] = '\0';
}

/* 生成时间串 "HH:MM:SS"，占 8 个字符 */
static void rtc_clock_str(char *buf, const rtc_time_t *t) {
    put_two(&buf[0], t->hour);
    buf[2] = ':';
    put_two(&buf[3], t->minute);
    buf[5] = ':';
    put_two(&buf[6], t->second);
    buf[8] = '\0';
}

/* 把当前时间画到 OLED */
static void rtc_show(void) {
    rtc_time_t t;
    char date[11];
    char clock[9];

    drv_rtc_get(&t);

    drv_display_clear();
    if (drv_rtc_is_valid()) {
        drv_display_line(0U, "DS3231 OK");
    } else {
        drv_display_line(0U, "DS3231 LOST");
    }

    rtc_date_str(date, &t);
    rtc_clock_str(clock, &t);
    drv_display_line(1U, date);
    drv_display_line(2U, clock);
    drv_display_line(3U, "K1:Set Time");
    drv_display_refresh();
}

/* 按 K1 后把时间写成 RTC_TEST_* 定义的固定值 */
static void rtc_set_default(void) {
    rtc_time_t t;

    t.year   = RTC_TEST_YEAR;
    t.month  = RTC_TEST_MONTH;
    t.day    = RTC_TEST_DAY;
    t.hour   = RTC_TEST_HOUR;
    t.minute = RTC_TEST_MINUTE;
    t.second = RTC_TEST_SECOND;

    drv_rtc_set(&t);
}

static void rtc_test(void) {
    key_id_t key = drv_input_scan();
    uint32_t now = drv_rtc_now();

    if (key == KEY_K1) {
        rtc_set_default();
        drv_indicator_show(IND_KEY);
        s_last_sec = now;
        rtc_show();
        return;
    }

    /* 秒数变化才刷新一次，避免主循环每个 tick 都重画 OLED */
    if (now != s_last_sec) {
        s_last_sec = now;
        rtc_show();
    }
}

void app_fsm_init(void) {
    s_state = ST_IDLE;
    s_last_sec = 0U;
}

void app_fsm_run(void) {
    rtc_test();

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
