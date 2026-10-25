#include "board_config.h"
#include "delay.h"

#include "bsp_gpio.h"
#include "bsp_spi.h"
#include "bsp_i2c.h"
#include "bsp_usart.h"

#include "drv_rfid.h"
#include "drv_rtc.h"
#include "drv_storage.h"
#include "drv_display.h"
#include "drv_input.h"
#include "drv_indicator.h"
#include "drv_net.h"

#include "proto_frame.h"

#include "app_config.h"
#include "app_auth.h"
#include "app_inventory.h"
#include "app_log.h"
#include "app_fsm.h"
#include "app_ui.h"

int main(void) {
    if (SysTick_Config(SystemCoreClock / 1000U) != 0U) {
        while (1) {
        }
    }
    delay_init();

    /* 1. 总线与板级外设 */
    bsp_gpio_init();
    bsp_spi_init();
    bsp_i2c_init();
    bsp_usart_init();

    bsp_usart_send_str(USART_PORT_DEBUG, "[BOOT] LabManager\r\n");

    /* 2. 设备抽象层 */
    drv_rtc_init();
    drv_storage_init();
    drv_display_init();
    drv_input_init();
    drv_indicator_init();
    drv_rfid_init();

    /* 3. 业务层：顺序固定，先配置、再权限、再台账、最后日志 */
    app_config_load();
    app_auth_init();
    app_inventory_init();
    app_log_init();

    /* 4. 网络：参数来自配置，回调交给协议层 */
    drv_net_init(g_sys_config.wifi_ssid, g_sys_config.wifi_pass,
                 g_sys_config.server_host, g_sys_config.server_port);
    drv_net_register_rx(proto_on_frame_received);

    app_fsm_init();
    app_ui_init();

    /* 5. 主循环：所有 poll 必须非阻塞 */
    for (;;) {
        uint32_t now = drv_rtc_now();

        drv_net_poll();
        drv_storage_poll();
        app_log_poll();
        app_inventory_poll(now);
        app_fsm_run();
        drv_indicator_poll();
    }
}
