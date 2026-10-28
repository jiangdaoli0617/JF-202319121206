#ifndef BSP_USART_H
#define BSP_USART_H

#include <stdint.h>

typedef enum {
    USART_PORT_DEBUG = 1,   /* USART2：PA2/PA3，接串口助手 */
    USART_PORT_WIFI  = 2    /* USART1：PA9/PA10，接 ESP8266 */
} usart_port_t;

void     bsp_usart_init(void);
void     bsp_usart_send(usart_port_t port, const uint8_t *data, uint16_t len);
void     bsp_usart_send_str(usart_port_t port, const char *s);
/* 非阻塞：把当前已收到的字节读入 buf，返回实际读取数 */
uint16_t bsp_usart_read(usart_port_t port, uint8_t *buf, uint16_t max);

#endif /* BSP_USART_H */
