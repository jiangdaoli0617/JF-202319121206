#include "bsp_esp8266.h"
#include "bsp_usart.h"

/* TODO: AT 指令状态机。
 * 建议实现顺序：
 *   1. AT            → OK
 *   2. AT+CWMODE=1   → OK
 *   3. AT+CWJAP=...  → WIFI GOT IP
 *   4. AT+CIPMUX=0   → OK
 *   5. AT+CIPSTART="TCP","host",port → CONNECT
 *   6. AT+CIPSEND=n  → > ，然后发 n 字节
 * 接收统一走 USART_PORT_WIFI，注意 AT 回显与 +IPD 数据混流。 */

void bsp_esp8266_init(void) {
}

uint8_t bsp_esp8266_cmd(const char *cmd, const char *expect, uint32_t timeout_ms) {
    (void)cmd;
    (void)expect;
    (void)timeout_ms;
    return 0U;
}

uint8_t bsp_esp8266_send_raw(const uint8_t *data, uint16_t len) {
    (void)data;
    (void)len;
    return 0U;
}

uint16_t bsp_esp8266_read(uint8_t *buf, uint16_t max) {
    return bsp_usart_read(USART_PORT_WIFI, buf, max);
}
