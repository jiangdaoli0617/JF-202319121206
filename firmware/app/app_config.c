#include "app_config.h"
#include <string.h>

sys_config_t g_sys_config;

static void copy_str(char *dst, uint16_t size, const char *src) {
    uint16_t i = 0U;

    if (size == 0U || dst == 0 || src == 0) {
        return;
    }
    while ((uint16_t)(i + 1U) < size && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

void app_config_load(void) {
    memset(&g_sys_config, 0, sizeof(g_sys_config));

    if (!drv_storage_load_config(&g_sys_config)) {
        copy_str(g_sys_config.device_id, (uint16_t)sizeof(g_sys_config.device_id),
                 "LAB-01-A");
        copy_str(g_sys_config.wifi_ssid, (uint16_t)sizeof(g_sys_config.wifi_ssid),
                 "");
        copy_str(g_sys_config.wifi_pass, (uint16_t)sizeof(g_sys_config.wifi_pass),
                 "");
        copy_str(g_sys_config.server_host,
                 (uint16_t)sizeof(g_sys_config.server_host), "192.168.1.100");
        g_sys_config.server_port = 9000U;
        g_sys_config.overdue_days = 7U;
    }

    if (g_sys_config.overdue_days == 0U) {
        g_sys_config.overdue_days = 7U;
    }
}

uint8_t app_config_save(void) {
    drv_storage_save_config(&g_sys_config);
    return 1U;
}
