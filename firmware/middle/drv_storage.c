#include "drv_storage.h"
#include "bsp_at24c256.h"

/* TODO: 按 docs/07 实现固定分区、A/B 双备份、环形日志队列。
 * 建议实现顺序：
 *   1. 配置块读写 + A/B 选择
 *   2. 卡表块
 *   3. 档案块
 *   4. 日志环形队列与上电恢复
 *   5. 分页写入与 flush
 * 在此之前，所有 load 返回 0，上层按「无有效数据」处理。 */

void drv_storage_init(void) {
    bsp_at24c256_init();
}

/* 自检用的临时区：放在 32 KB 的最末尾，避开将来正式数据区（从 0x0000 开始） */
#define STORAGE_TEST_ADDR   0x7F00U
#define STORAGE_TEST_LEN    16U

uint8_t drv_storage_selftest(void) {
    uint8_t wr[STORAGE_TEST_LEN];
    uint8_t rd[STORAGE_TEST_LEN];
    uint8_t i;

    /* 第一步：确认器件在线（读一个字节看有没有应答） */
    if (!bsp_at24c256_probe()) {
        return 0U;
    }

    /* 第二步：生成一组有规律的测试数据，写入 */
    for (i = 0U; i < STORAGE_TEST_LEN; i++) {
        wr[i] = (uint8_t)(0xA5U ^ i);
    }
    if (!bsp_at24c256_write(STORAGE_TEST_ADDR, wr, STORAGE_TEST_LEN)) {
        return 0U;
    }

    /* 第三步：读回逐字节比对，全部一致才算通过 */
    if (!bsp_at24c256_read(STORAGE_TEST_ADDR, rd, STORAGE_TEST_LEN)) {
        return 0U;
    }
    for (i = 0U; i < STORAGE_TEST_LEN; i++) {
        if (rd[i] != wr[i]) {
            return 0U;
        }
    }

    return 1U;
}

/* 上电次数保存位置：0x7FF0 起 4 字节
 *   [0][1] 魔数 0x5A 0xA5（用来判断这块区域是否被初始化过）
 *   [2][3] 计数，大端（高字节在前），与项目整体约定一致 */
#define STORAGE_BOOT_ADDR   0x7FF0U
#define STORAGE_BOOT_MAGIC0 0x5AU
#define STORAGE_BOOT_MAGIC1 0xA5U

uint16_t drv_storage_boot_count(void) {
    uint8_t  buf[4];
    uint16_t count;

    /* 读出上次保存的内容；读不到就当作 0，不影响启动 */
    if (!bsp_at24c256_read(STORAGE_BOOT_ADDR, buf, 4U)) {
        return 0U;
    }

    if ((buf[0] != STORAGE_BOOT_MAGIC0) || (buf[1] != STORAGE_BOOT_MAGIC1)) {
        count = 0U;                       /* 首次上电，或该区域还是空白 */
    } else {
        count = (uint16_t)(((uint16_t)buf[2] << 8) | (uint16_t)buf[3]);
        if (count >= 60000U) {
            count = 0U;                   /* 防止无符号溢出后跳回小数 */
        }
    }
    count++;

    buf[0] = STORAGE_BOOT_MAGIC0;
    buf[1] = STORAGE_BOOT_MAGIC1;
    buf[2] = (uint8_t)(count >> 8);
    buf[3] = (uint8_t)(count & 0xFFU);
    if (!bsp_at24c256_write(STORAGE_BOOT_ADDR, buf, 4U)) {
        return 0U;
    }

    return count;
}

uint8_t drv_storage_load_config(sys_config_t *cfg) {
    (void)cfg;
    return 0U;
}

uint16_t drv_storage_load_users(user_card_t *out, uint16_t max) {
    (void)out;
    (void)max;
    return 0U;
}

uint16_t drv_storage_load_equipment(equip_record_t *out, uint16_t max) {
    (void)out;
    (void)max;
    return 0U;
}

uint16_t drv_storage_load_logs(log_entry_t *out, uint16_t max) {
    (void)out;
    (void)max;
    return 0U;
}

void drv_storage_save_config(const sys_config_t *cfg) {
    (void)cfg;
}

uint8_t drv_storage_save_users(const user_card_t *list, uint16_t count) {
    (void)list;
    (void)count;
    return 0U;
}

uint8_t drv_storage_save_equipment(const equip_record_t *list, uint16_t count) {
    (void)list;
    (void)count;
    return 0U;
}

uint8_t drv_storage_append_log(const log_entry_t *e) {
    (void)e;
    return 0U;
}

void drv_storage_ack_log(void) {
}

void drv_storage_poll(void) {
}

uint8_t drv_storage_flush(void) {
    return 1U;
}
