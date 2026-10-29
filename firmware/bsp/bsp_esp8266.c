#include "bsp_esp8266.h"
#include "bsp_usart.h"
#include "board_config.h"

#include <string.h>

/* ============================================================================
 * ESP-01S（ESP8266）AT 指令驱动
 *
 * 模块只管「发一串字符、等一串期望的字符回显」，不做业务判断。
 * 具体的连 WiFi、建 TCP、发数据由上层 drv_net 按状态机串联。
 *
 * 接收采用一个 256 字节的滑动缓冲：
 *   串口收到的每个字节都先攒进 s_rx，AT 命令的应答和 +IPD 数据都在里面。
 *   bsp_esp8266_cmd() 只在本条命令发出之后的字节里找期望串，
 *   因此缓冲区里还没被取走的 +IPD 数据不会影响判断。
 *   缓冲区满时丢掉最旧的一半，保证最新的应答一定还在。
 * ============================================================================ */

#define ESP_RX_BUF_SIZE   256U

/* AT 指令最长等待时间。模块回复通常在 100ms 内，2 秒足够覆盖连 WiFi 的情况 */
#define ESP_CMD_TIMEOUT_MS 2000U

static char     s_rx[ESP_RX_BUF_SIZE];
static uint16_t s_rx_len;
static uint8_t  s_rx_trimmed;      /* 缓冲被压缩过：旧数据已丢弃，查找起点要从 0 重新算 */

/* 把串口里已经收到的字节搬进缓冲 */
static void esp_rx_poll(void) {
    uint8_t b;

    while (bsp_usart_read(USART_PORT_WIFI, &b, 1U) == 1U) {
        if (s_rx_len >= (ESP_RX_BUF_SIZE - 1U)) {
            /* 缓冲满了：保留最近的一半，丢掉较早的一半 */
            uint16_t keep = ESP_RX_BUF_SIZE / 2U;
            memmove(s_rx, s_rx + (s_rx_len - keep), keep);
            s_rx_len = keep;
            s_rx_trimmed = 1U;
        }
        s_rx[s_rx_len] = (char)b;
        s_rx_len++;
    }
    s_rx[s_rx_len] = '\0';
}

void bsp_esp8266_init(void) {
    uint8_t dummy;
    uint32_t start;

    /* 上电时串口里可能残留上次运行的字节，先读空 */
    while (bsp_usart_read(USART_PORT_WIFI, &dummy, 1U) == 1U) {
    }

    s_rx_len = 0U;
    s_rx[0] = '\0';
    s_rx_trimmed = 0U;

    /* 给模块一点时间完成上电自检，避免第一条 AT 被吞掉 */
    start = g_sys_tick_ms;
    while ((uint32_t)(g_sys_tick_ms - start) < 100U) {
    }
}

uint8_t bsp_esp8266_cmd(const char *cmd, const char *expect, uint32_t timeout_ms) {
    uint32_t start;
    uint16_t base;

    /* 先看看有没有上次没取走的数据，从这里之后开始找 */
    esp_rx_poll();
    base = s_rx_trimmed ? 0U : s_rx_len;

    if (cmd != 0) {
        bsp_usart_send_str(USART_PORT_WIFI, cmd);
        bsp_usart_send_str(USART_PORT_WIFI, "\r\n");
    }

    /* expect 传空表示「只发不等」 */
    if (expect == 0) {
        return 1U;
    }

    if (timeout_ms == 0U) {
        timeout_ms = ESP_CMD_TIMEOUT_MS;
    }

    start = g_sys_tick_ms;
    for (;;) {
        esp_rx_poll();
        if (s_rx_trimmed) {
            base = 0U;
            s_rx_trimmed = 0U;
        }
        if (base <= s_rx_len && strstr(&s_rx[base], expect) != 0) {
            return 1U;
        }
        if ((uint32_t)(g_sys_tick_ms - start) >= timeout_ms) {
            return 0U;
        }
    }
}

uint8_t bsp_esp8266_send_raw(const uint8_t *data, uint16_t len) {
    if (data == 0 || len == 0U) {
        return 0U;
    }
    bsp_usart_send(USART_PORT_WIFI, data, len);
    return 1U;
}

uint16_t bsp_esp8266_read(uint8_t *buf, uint16_t max) {
    uint16_t n;

    if (buf == 0 || max == 0U) {
        return 0U;
    }

    esp_rx_poll();

    n = (s_rx_len < max) ? s_rx_len : max;
    if (n == 0U) {
        return 0U;
    }

    memcpy(buf, s_rx, n);
    s_rx_len = (uint16_t)(s_rx_len - n);
    memmove(s_rx, s_rx + n, s_rx_len);
    s_rx[s_rx_len] = '\0';

    return n;
}
