#include "drv_rfid.h"
#include "bsp_rc522.h"
#include "board_config.h"

void drv_rfid_init(void) {
    /* SPI 总线由 main 统一初始化，这里只做器件级复位 */
    bsp_rc522_init();
}

uint8_t drv_rfid_poll(rfid_card_t *card) {
    static uint32_t s_last_uid  = 0U;
    static uint32_t s_last_ms   = 0U;
    static uint8_t  s_have_last = 0U;

    uint32_t uid;
    uint32_t now;

    if (card == 0) {
        return 0U;
    }
    if (!bsp_rc522_read_uid(&uid)) {
        return 0U;
    }

    now = g_sys_tick_ms;
    if (s_have_last && uid == s_last_uid &&
        (uint32_t)(now - s_last_ms) < RFID_DEDUP_MS) {
        return 0U;
    }

    s_last_uid  = uid;
    s_last_ms   = now;
    s_have_last = 1U;

    card->uid     = uid;
    card->uid_len = 4U;
    card->valid   = 1U;
    return 1U;
}

void drv_rfid_antenna(uint8_t on) {
    bsp_rc522_antenna(on);
}
