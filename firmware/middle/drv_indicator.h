#ifndef DRV_INDICATOR_H
#define DRV_INDICATOR_H

#include <stdint.h>

typedef enum {
    IND_LEND_OK = 0,     /* 借还成功：绿亮 + 短鸣 */
    IND_REJECT,          /* 业务拒绝：红闪两次 + 双短鸣 */
    IND_UNKNOWN_CARD,    /* 未授权卡：红长亮 + 长鸣 */
    IND_ERROR,           /* 内部异常：红快闪 + 急促鸣 */
    IND_NET_DOWN,        /* 断网：红慢闪，无声音，循环 */
    IND_IDLE             /* 清除所有指示 */
} ind_event_t;

void drv_indicator_init(void);
void drv_indicator_show(ind_event_t ev);   /* 非阻塞，由主循环驱动 */
void drv_indicator_poll(void);

#endif /* DRV_INDICATOR_H */
