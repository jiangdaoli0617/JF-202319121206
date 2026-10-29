#ifndef BSP_ESP8266_H
#define BSP_ESP8266_H

#include <stdint.h>

void     bsp_esp8266_init(void);

/* 发送一条 AT 指令（自动补 "\r\n"），并在 timeout_ms 内等待回复中出现 expect 子串。
 * 返回 1 = 命中，0 = 超时。
 *   expect 传 0 表示「只发不等」，立即返回 1。
 *   timeout_ms 传 0 使用默认 2000ms。
 * 注意：这是阻塞调用，最长阻塞 timeout_ms。连接 WiFi、建立 TCP 这类慢指令
 *       只在上层重连状态机里偶尔调用，不要每轮主循环都调。 */
uint8_t  bsp_esp8266_cmd(const char *cmd, const char *expect, uint32_t timeout_ms);

/* 原样发送数据（用于 AT+CIPSEND 之后的报文），不加 "\r\n" */
uint8_t  bsp_esp8266_send_raw(const uint8_t *data, uint16_t len);

/* 取走已收到的原始数据（AT 回显与 +IPD 数据都在里面），返回实际字节数 */
uint16_t bsp_esp8266_read(uint8_t *buf, uint16_t max);

#endif /* BSP_ESP8266_H */
