#ifndef DRV_RTC_H
#define DRV_RTC_H

#include <stdint.h>

typedef struct {
    uint16_t year;      /* 完整年份，如 2026 */
    uint8_t  month;
    uint8_t  day;
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  second;
    uint32_t unix;      /* Unix 秒（UTC） */
} rtc_time_t;

void     drv_rtc_init(void);
void     drv_rtc_get(rtc_time_t *t);
/* 使用 t 的年月日时分秒写 DS3231，并清除 OSF 标志；t->unix 会被重算 */
void     drv_rtc_set(const rtc_time_t *t);
uint8_t  drv_rtc_is_valid(void);   /* 0 = 掉电后时间不可信 */
uint32_t drv_rtc_now(void);        /* Unix 秒；内部用 SysTick 缓存，避免频繁 I2C */

#endif /* DRV_RTC_H */
