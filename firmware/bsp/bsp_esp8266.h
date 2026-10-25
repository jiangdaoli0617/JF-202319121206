#ifndef BSP_ESP8266_H
#define BSP_ESP8266_H

#include <stdint.h>

void     bsp_esp8266_init(void);
/* 发送 AT 指令并等待 expect 子串；返回 1 命中，0 超时。
 * 内部使用 USART_PORT_WIFI，不阻塞主循环以外的调用者。 */
uint8_t  bsp_esp8266_cmd(const char *cmd, const char *expect, uint32_t timeout_ms);
uint8_t  bsp_esp8266_send_raw(const uint8_t *data, uint16_t len);
uint16_t bsp_esp8266_read(uint8_t *buf, uint16_t max);

#endif /* BSP_ESP8266_H */
