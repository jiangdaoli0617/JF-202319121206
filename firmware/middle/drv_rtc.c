#include "drv_rtc.h"
#include "bsp_ds3231.h"
#include "board_config.h"
#include <string.h>

#define DS3231_REG_TIME    0x00
#define DS3231_REG_STATUS  0x0F
#define DS3231_OSF_MASK    0x80U

static uint32_t s_base_unix = 0U;
static uint32_t s_base_tick = 0U;

static uint8_t bcd_to_bin(uint8_t v) {
    return (uint8_t)(((v >> 4) * 10U) + (v & 0x0FU));
}

static uint8_t bin_to_bcd(uint8_t v) {
    return (uint8_t)(((v / 10U) << 4) | (v % 10U));
}

/* 公历日期 → 1970-01-01 起的天数（Howard Hinnant 算法） */
static uint32_t days_from_civil(int y, unsigned m, unsigned d) {
    int era;
    unsigned yoe;
    unsigned doy;
    unsigned doe;

    y -= (m <= 2U) ? 1 : 0;
    era = (y >= 0 ? y : y - 399) / 400;
    yoe = (unsigned)(y - era * 400);
    doy = (153U * (m > 2U ? m - 3U : m + 9U) + 2U) / 5U + d - 1U;
    doe = yoe * 365U + yoe / 4U - yoe / 100U + doy;
    return (uint32_t)(era * 146097 + (int)doe - 719468);
}

static uint32_t to_unix(uint16_t year, uint8_t month, uint8_t day,
                        uint8_t hour, uint8_t minute, uint8_t second) {
    uint32_t days = days_from_civil((int)year, month, day);

    return days * 86400UL + (uint32_t)hour * 3600UL +
           (uint32_t)minute * 60UL + (uint32_t)second;
}

void drv_rtc_init(void) {
    rtc_time_t t;

    bsp_ds3231_init();
    drv_rtc_get(&t);
    if (t.unix != 0U) {
        s_base_unix = t.unix;
        s_base_tick = g_sys_tick_ms;
    }
}

void drv_rtc_get(rtc_time_t *t) {
    uint8_t raw[7];
    uint8_t hour;

    if (t == 0) {
        return;
    }
    memset(t, 0, sizeof(*t));

    if (!bsp_ds3231_read_raw(DS3231_REG_TIME, raw, 7U)) {
        return;
    }

    t->second = bcd_to_bin((uint8_t)(raw[0] & 0x7FU));
    t->minute = bcd_to_bin((uint8_t)(raw[1] & 0x7FU));

    if (raw[2] & 0x40U) {           /* 12 小时制 */
        hour = bcd_to_bin((uint8_t)(raw[2] & 0x1FU));
        hour = (uint8_t)(hour % 12U);
        if (raw[2] & 0x20U) {       /* PM */
            hour = (uint8_t)(hour + 12U);
        }
    } else {                        /* 24 小时制 */
        hour = bcd_to_bin((uint8_t)(raw[2] & 0x3FU));
    }
    t->hour = hour;

    t->day   = bcd_to_bin((uint8_t)(raw[4] & 0x3FU));
    t->month = bcd_to_bin((uint8_t)(raw[5] & 0x1FU));
    t->year  = (uint16_t)(2000U + bcd_to_bin(raw[6]));
    if (raw[5] & 0x80U) {           /* 世纪位 */
        t->year = (uint16_t)(t->year + 100U);
    }

    t->unix = to_unix(t->year, t->month, t->day,
                      t->hour, t->minute, t->second);
}

void drv_rtc_set(const rtc_time_t *t) {
    uint8_t raw[7];
    uint8_t status;

    if (t == 0) {
        return;
    }

    raw[0] = bin_to_bcd(t->second);
    raw[1] = bin_to_bcd(t->minute);
    raw[2] = bin_to_bcd(t->hour);       /* 固定 24 小时制 */
    raw[3] = 1U;                         /* 星期寄存器不影响计时 */
    raw[4] = bin_to_bcd(t->day);
    raw[5] = bin_to_bcd(t->month);
    raw[6] = bin_to_bcd((uint8_t)(t->year % 100U));
    if (t->year >= 2100U) {
        raw[5] |= 0x80U;
    }

    if (!bsp_ds3231_write_raw(DS3231_REG_TIME, raw, 7U)) {
        return;
    }

    /* 写入成功后清除 OSF，表示时间重新可信 */
    if (bsp_ds3231_read_raw(DS3231_REG_STATUS, &status, 1U)) {
        status &= (uint8_t)~DS3231_OSF_MASK;
        bsp_ds3231_write_raw(DS3231_REG_STATUS, &status, 1U);
    }

    s_base_unix = to_unix(t->year, t->month, t->day,
                          t->hour, t->minute, t->second);
    s_base_tick = g_sys_tick_ms;
}

uint8_t drv_rtc_is_valid(void) {
    return bsp_ds3231_oscillator_ok();
}

uint32_t drv_rtc_now(void) {
    uint32_t elapsed = (uint32_t)(g_sys_tick_ms - s_base_tick) / 1000U;

    return s_base_unix + elapsed;
}
