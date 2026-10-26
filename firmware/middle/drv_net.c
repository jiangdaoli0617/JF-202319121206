#include "drv_net.h"
#include "bsp_esp8266.h"

/* TODO: AT 指令重连状态机。
 * 设计约束：
 *   - drv_net_poll() 必须非阻塞，等待 OK/CONNECT 一律用时间戳状态机；
 *   - 收到 +IPD 数据后拼成完整帧，再回调 s_rx_cb；
 *   - 断线后指数退避重连，期间状态为 NET_STATE_ERROR；
 *   - 上层只通过 drv_net_send() 发完整帧，不感知 AT 指令。 */

static net_rx_cb_t s_rx_cb = 0;
static net_state_t s_state = NET_STATE_IDLE;

void drv_net_init(const char *ssid, const char *pass,
                  const char *host, uint16_t port) {
    (void)ssid;
    (void)pass;
    (void)host;
    (void)port;

    bsp_esp8266_init();
    s_state = NET_STATE_IDLE;
}

void drv_net_register_rx(net_rx_cb_t cb) {
    s_rx_cb = cb;
}

void drv_net_poll(void) {
}

uint8_t drv_net_send(const uint8_t *frame, uint16_t len) {
    (void)frame;
    (void)len;
    return 0U;
}

net_state_t drv_net_state(void) {
    return s_state;
}
