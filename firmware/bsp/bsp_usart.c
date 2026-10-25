#include "bsp_usart.h"
#include "board_config.h"
#include <string.h>

static USART_TypeDef *port_to_usart(usart_port_t port) {
    return (port == USART_PORT_WIFI) ? WIFI_USART : DBG_USART;
}

static void usart_gpio_init(void) {
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA |
                           RCC_APB2Periph_AFIO, ENABLE);

    /* USART1 TX / USART2 TX：复用推挽 */
    gpio.GPIO_Pin   = DBG_TX_PIN | WIFI_TX_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    /* USART1 RX / USART2 RX：浮空输入 */
    gpio.GPIO_Pin  = DBG_RX_PIN | WIFI_RX_PIN;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);
}

static void usart_config(USART_TypeDef *usart, uint32_t baud) {
    USART_InitTypeDef cfg;

    cfg.USART_BaudRate            = baud;
    cfg.USART_WordLength          = USART_WordLength_8b;
    cfg.USART_StopBits            = USART_StopBits_1;
    cfg.USART_Parity              = USART_Parity_No;
    cfg.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    cfg.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(usart, &cfg);

    USART_Cmd(usart, ENABLE);
}

void bsp_usart_init(void) {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

    usart_gpio_init();
    usart_config(DBG_USART,  DBG_USART_BAUD);
    usart_config(WIFI_USART, WIFI_USART_BAUD);
}

void bsp_usart_send(usart_port_t port, const uint8_t *data, uint16_t len) {
    USART_TypeDef *usart = port_to_usart(port);
    uint16_t i;

    for (i = 0U; i < len; i++) {
        while (USART_GetFlagStatus(usart, USART_FLAG_TXE) == RESET) {
        }
        USART_SendData(usart, data[i]);
    }
    while (USART_GetFlagStatus(usart, USART_FLAG_TC) == RESET) {
    }
}

void bsp_usart_send_str(usart_port_t port, const char *s) {
    if (s == 0) {
        return;
    }
    bsp_usart_send(port, (const uint8_t *)s, (uint16_t)strlen(s));
}

uint16_t bsp_usart_read(usart_port_t port, uint8_t *buf, uint16_t max) {
    USART_TypeDef *usart = port_to_usart(port);
    uint16_t n = 0U;

    while (n < max &&
           USART_GetFlagStatus(usart, USART_FLAG_RXNE) != RESET) {
        buf[n++] = (uint8_t)USART_ReceiveData(usart);
    }
    return n;
}
