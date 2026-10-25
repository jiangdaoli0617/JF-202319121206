#include "drv_input.h"
#include "bsp_gpio.h"
#include "board_config.h"
#include <string.h>

static uint8_t  s_last_raw[KEY_COUNT];
static uint8_t  s_reported[KEY_COUNT];
static uint32_t s_change_ms[KEY_COUNT];

void drv_input_init(void) {
    memset(s_last_raw, 0, sizeof(s_last_raw));
    memset(s_reported, 0, sizeof(s_reported));
    memset(s_change_ms, 0, sizeof(s_change_ms));
}

key_id_t drv_input_scan(void) {
    uint32_t now = g_sys_tick_ms;
    uint8_t  i;

    for (i = 0U; i < KEY_COUNT; i++) {
        uint8_t raw = bsp_key_raw(i);

        if (raw != s_last_raw[i]) {
            s_last_raw[i] = raw;
            s_change_ms[i] = now;
        } else if ((uint32_t)(now - s_change_ms[i]) >= KEY_DEBOUNCE_MS) {
            if (raw != 0U && s_reported[i] == 0U) {
                s_reported[i] = 1U;
                return (key_id_t)(KEY_K1 + i);
            }
            if (raw == 0U) {
                s_reported[i] = 0U;
            }
        }
    }
    return KEY_NONE;
}
