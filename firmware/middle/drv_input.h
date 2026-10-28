#ifndef DRV_INPUT_H
#define DRV_INPUT_H

#include <stdint.h>

typedef enum { KEY_NONE = 0, KEY_K1, KEY_K2, KEY_K3, KEY_K4, KEY_K5 } key_id_t;

void     drv_input_init(void);
/* 已含 20 ms 消抖与松手检测；每次只返回一个按下事件 */
key_id_t drv_input_scan(void);

#endif /* DRV_INPUT_H */
