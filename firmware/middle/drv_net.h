#ifndef DRV_NET_H
#define DRV_NET_H

#include <stdint.h>

typedef enum {
    NET_STATE_IDLE = 0,
    NET_STATE_CONNECTING,
    NET_STATE_ONLINE,
    NET_STATE_ERROR
} net_state_t;

typedef void (*net_rx_cb_t)(const uint8_t *frame, uint16_t len);

void         drv_net_init(const char *ssid, const char *pass,
                          const char *host, uint16_t port);
void         drv_net_register_rx(net_rx_cb_t cb);
void         drv_net_poll(void);
uint8_t      drv_net_send(const uint8_t *frame, uint16_t len);
net_state_t  drv_net_state(void);

#endif /* DRV_NET_H */
