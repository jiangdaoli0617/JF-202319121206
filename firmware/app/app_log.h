#ifndef APP_LOG_H
#define APP_LOG_H

#include "drv_storage.h"

void     app_log_init(void);                  /* 恢复未确认记录并续接 LOG_SEQ */
uint8_t  app_log_push(const log_entry_t *e);  /* 队列满返回 0；seq 由内部赋值 */
void     app_log_poll(void);                  /* 发送/重传/补传 */
uint16_t app_log_pending(void);

#endif /* APP_LOG_H */
