#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "drv_storage.h"

/* 全局配置。掉电存储实现后由 drv_storage_load_config 填充；
 * 当前读不到时使用内置默认值，保证空片上电也能运行。 */
extern sys_config_t g_sys_config;

void    app_config_load(void);
uint8_t app_config_save(void);

#endif /* APP_CONFIG_H */
