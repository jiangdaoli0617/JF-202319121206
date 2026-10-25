#ifndef APP_INVENTORY_H
#define APP_INVENTORY_H

#include "drv_storage.h"

void     app_inventory_init(void);
/* 上位机下发档案时调用。实现须先应用服务端档案，再重放本地未确认日志，
 * 避免覆盖断网期间的借还；返回写入条数。 */
uint16_t app_inventory_bulk_load(const equip_record_t *list, uint16_t count);
uint8_t  app_inventory_find(uint16_t equip_id, equip_record_t *out);
uint8_t  app_inventory_lend(uint16_t equip_id, uint32_t user_card, uint32_t now);
uint8_t  app_inventory_return(uint16_t equip_id, uint32_t user_card, uint32_t now);
uint16_t app_inventory_count_in_stock(void);
/* 主循环调用；内部每 60 s 扫描一次超期 */
void     app_inventory_poll(uint32_t now);
uint16_t app_inventory_count_overdue(void);
uint16_t app_inventory_get_overdue_list(equip_record_t *out, uint16_t max);

#endif /* APP_INVENTORY_H */
