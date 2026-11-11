#include "drv_storage.h"
#include "bsp_at24c256.h"
#include "proto_crc.h"

/* ============================================================================
 * AT24C256 存储层
 *
 * 器件：AT24C256，32 KB，I²C 地址 0x50，片内地址 2 字节
 *
 * 分区布局（每个副本占 1 KB，日志占 2 KB）：
 *   0x0000  人员卡表 A 副本
 *   0x0400  人员卡表 B 副本
 *   0x0800  器材表   A 副本
 *   0x0C00  器材表   B 副本
 *   0x1000  系统配置 A 副本
 *   0x1400  系统配置 B 副本
 *   0x1800  日志环形队列（单副本，追加写）
 *   0x7F00  自检临时区（16 B）
 *   0x7FF0  上电计数（4 B）
 *
 * A/B 双备份的作用：
 *   保存时永远写「较旧」的那一份，写成功后才改序号。如果写到一半断电，
 *   旧的那份仍然完整可用，上电时读两份、挑校验通过且序号大的那份即可，
 *   从而保证 EEPROM 写周期内掉电不会丢数据。
 *
 * 每个副本的头部 8 字节：
 *   [0][1] 魔数 0x4C 0x4D（'L' 'M'）
 *   [2][3] 序号 seq（大端，越大表示越新）
 *   [4][5] 数据长度 len（大端）
 *   [6][7] 数据 CRC16-MODBUS（大端）
 * ============================================================================ */

/* ---------- 分区地址 ---------- */
#define ADDR_USERS_A     0x0000U
#define ADDR_USERS_B     0x0400U
#define ADDR_EQUIP_A     0x0800U
#define ADDR_EQUIP_B     0x0C00U
#define ADDR_CONFIG_A    0x1000U
#define ADDR_CONFIG_B    0x1400U
#define ADDR_LOGS        0x1800U

/* ---------- 自检区与上电计数 ---------- */
#define STORAGE_TEST_ADDR   0x7F00U
#define STORAGE_TEST_LEN    16U
#define STORAGE_BOOT_ADDR   0x7FF0U
#define STORAGE_BOOT_MAGIC0 0x5AU
#define STORAGE_BOOT_MAGIC1 0xA5U

/* ---------- 块头与记录尺寸 ---------- */
#define STOR_HDR_SIZE    8U
#define STOR_MAGIC0      0x4CU
#define STOR_MAGIC1      0x4DU

#define REC_USER_SIZE    5U    /* 卡号 4 + 角色 1 */
#define REC_EQUIP_SIZE   15U   /* 编号 2 + 状态 1 + 持卡人 4 + 借出时间 4 + 应还时间 4 */
#define REC_LOG_SIZE     20U   /* 序号 4 + 操作 1 + 结果 1 + 编号 2 + 卡号 4 + 时间戳 4 + 应还 4 */
#define CFG_SIZE         80U   /* 见 pack_config() */

/* ---------- 日志环形队列 ---------- */
#define LOG_SLOTS        36U                     /* 最多保存 36 条 */
#define LOG_HDR_SIZE     10U                     /* 魔数2 头2 数2 确认2 CRC2 */

/* ---------- 小工具：大端读写 ---------- */
static void put_u16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)(v & 0xFFU);
}

static uint16_t get_u16(const uint8_t *p) {
    return (uint16_t)(((uint16_t)p[0] << 8) | (uint16_t)p[1]);
}

static void put_u32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)((v >> 16) & 0xFFU);
    p[2] = (uint8_t)((v >> 8) & 0xFFU);
    p[3] = (uint8_t)(v & 0xFFU);
}

static uint32_t get_u32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

/* 定长字符串拷贝：不足补 0，保证同一份配置每次打包的字节完全一致 */
static void copy_str(char *dst, uint16_t dst_size, const char *src) {
    uint16_t i = 0U;

    while ((i + 1U < dst_size) && (src != 0) && (src[i] != '\0')) {
        dst[i] = src[i];
        i++;
    }
    while (i < dst_size) {
        dst[i] = '\0';
        i++;
    }
}

/* ---------------------------------------------------------------- 块读写 */

/* 只读头部、校验魔数，回填 len / seq（不校验数据 CRC，用于快速判断新旧） */
static uint8_t block_header(uint16_t base, uint16_t *len, uint16_t *seq) {
    uint8_t hdr[STOR_HDR_SIZE];

    if (!bsp_at24c256_read(base, hdr, STOR_HDR_SIZE)) {
        return 0U;
    }
    if ((hdr[0] != STOR_MAGIC0) || (hdr[1] != STOR_MAGIC1)) {
        return 0U;
    }
    *len = get_u16(&hdr[4]);
    *seq = get_u16(&hdr[2]);
    return (*len != 0U) ? 1U : 0U;
}

/* 读一个副本：校验魔数与 CRC，成功返回 1 并回填 len / seq */
static uint8_t block_read(uint16_t base, uint8_t *data, uint16_t cap,
                          uint16_t *len, uint16_t *seq) {
    uint8_t  hdr[STOR_HDR_SIZE];
    uint16_t dlen;
    uint16_t crc;

    if (!bsp_at24c256_read(base, hdr, STOR_HDR_SIZE)) {
        return 0U;
    }
    if ((hdr[0] != STOR_MAGIC0) || (hdr[1] != STOR_MAGIC1)) {
        return 0U;                        /* 空白区或从未写过 */
    }

    dlen = get_u16(&hdr[4]);
    if ((dlen == 0U) || (dlen > cap)) {
        return 0U;
    }

    if (!bsp_at24c256_read((uint16_t)(base + STOR_HDR_SIZE), data, dlen)) {
        return 0U;
    }

    crc = proto_crc16(data, dlen);
    if (crc != get_u16(&hdr[6])) {
        return 0U;                        /* 数据损坏 */
    }

    *len = dlen;
    *seq = get_u16(&hdr[2]);
    return 1U;
}

/* 写一个副本：先写数据、最后写头部，再读回头部确认 */
static uint8_t block_write(uint16_t base, const uint8_t *data,
                           uint16_t len, uint16_t seq) {
    uint8_t  hdr[STOR_HDR_SIZE];
    uint8_t  back[STOR_HDR_SIZE];
    uint16_t crc = proto_crc16(data, len);

    /* 先写数据，最后写头部。
     * 这样即使数据写了一半断电，头部仍是旧的（或无效），上电时会被判为
     * 损坏，从而回退到另一份完好副本，不会读到半新半旧的混合数据。 */
    if (!bsp_at24c256_write((uint16_t)(base + STOR_HDR_SIZE), data, len)) {
        return 0U;
    }

    hdr[0] = STOR_MAGIC0;
    hdr[1] = STOR_MAGIC1;
    put_u16(&hdr[2], seq);
    put_u16(&hdr[4], len);
    put_u16(&hdr[6], crc);

    if (!bsp_at24c256_write(base, hdr, STOR_HDR_SIZE)) {
        return 0U;
    }

    /* 读回头部做一次确认，避免「写了但器件实际没写进去」 */
    if (!bsp_at24c256_read(base, back, STOR_HDR_SIZE)) {
        return 0U;
    }
    if ((back[0] != STOR_MAGIC0) || (back[1] != STOR_MAGIC1) ||
        (get_u16(&back[4]) != len) || (get_u16(&back[6]) != crc)) {
        return 0U;
    }
    return 1U;
}

/* 从 A/B 两个副本中挑出「校验通过且序号更大」的那份。
 * 返回 1 表示找到有效数据，*use_b 指示数据取自 B 副本。 */
static uint8_t block_pick(uint16_t addr_a, uint16_t addr_b,
                          uint8_t *data, uint16_t cap,
                          uint16_t *len, uint16_t *seq, uint8_t *use_b) {
    uint16_t len_a = 0U, seq_a = 0U;
    uint16_t len_b = 0U, seq_b = 0U;
    uint8_t  ok_a = block_header(addr_a, &len_a, &seq_a);
    uint8_t  ok_b = block_header(addr_b, &len_b, &seq_b);
    uint8_t  first_b;

    if (!ok_a && !ok_b) {
        return 0U;                        /* 两份都没有有效数据 */
    }

    /* 两份都在时取序号大的（序号大的就是最后写入的那份） */
    if (ok_a && ok_b) {
        first_b = (seq_b > seq_a) ? 1U : 0U;
    } else {
        first_b = ok_b ? 1U : 0U;
    }

    /* 先试较新的那份；数据 CRC 不通过就退回另一份 */
    if (first_b) {
        if (block_read(addr_b, data, cap, len, seq)) { *use_b = 1U; return 1U; }
        if (ok_a && block_read(addr_a, data, cap, len, seq)) { *use_b = 0U; return 1U; }
    } else {
        if (block_read(addr_a, data, cap, len, seq)) { *use_b = 0U; return 1U; }
        if (ok_b && block_read(addr_b, data, cap, len, seq)) { *use_b = 1U; return 1U; }
    }
    return 0U;
}

/* 把数据写进「较旧」的那个副本，序号在旧序号基础上 +1 */
static uint8_t block_save(uint16_t addr_a, uint16_t addr_b,
                          const uint8_t *data, uint16_t len) {
    uint16_t len_a = 0U, seq_a = 0U;
    uint16_t len_b = 0U, seq_b = 0U;
    uint8_t  ok_a = block_header(addr_a, &len_a, &seq_a);
    uint8_t  ok_b = block_header(addr_b, &len_b, &seq_b);

    (void)len_a;
    (void)len_b;

    if (ok_a && ok_b) {
        if (seq_a >= seq_b) {
            return block_write(addr_b, data, len, (uint16_t)(seq_a + 1U));
        }
        return block_write(addr_a, data, len, (uint16_t)(seq_b + 1U));
    }
    if (ok_a) {
        return block_write(addr_b, data, len, (uint16_t)(seq_a + 1U));
    }
    if (ok_b) {
        return block_write(addr_a, data, len, (uint16_t)(seq_b + 1U));
    }
    /* 两份都无效（首次使用）：从 A 开始写，序号 1 */
    return block_write(addr_a, data, len, 1U);
}

/* ---------------------------------------------------------------- 打包 */

static void pack_user(uint8_t *p, const user_card_t *u) {
    put_u32(&p[0], u->card);
    p[4] = u->role;
}

static void unpack_user(const uint8_t *p, user_card_t *u) {
    u->card = get_u32(&p[0]);
    u->role = p[4];
}

static void pack_equip(uint8_t *p, const equip_record_t *e) {
    put_u16(&p[0], e->equip_id);
    p[2] = e->status;
    put_u32(&p[3], e->holder_card);
    put_u32(&p[7], e->lend_time);
    put_u32(&p[11], e->due_time);
    /* name[] 只在 RAM 中使用，不持久化 */
}

static void unpack_equip(const uint8_t *p, equip_record_t *e) {
    e->equip_id   = get_u16(&p[0]);
    e->status     = p[2];
    e->holder_card = get_u32(&p[3]);
    e->lend_time  = get_u32(&p[7]);
    e->due_time   = get_u32(&p[11]);
    e->name[0]    = '\0';
}

static void pack_log(uint8_t *p, const log_entry_t *l) {
    put_u32(&p[0],  l->seq);
    p[4]  = l->op_type;
    p[5]  = l->result;
    put_u16(&p[6], l->equip_id);
    put_u32(&p[8], l->user_card);
    put_u32(&p[12], l->timestamp);
    put_u32(&p[16], l->due_time);
}

static void unpack_log(const uint8_t *p, log_entry_t *l) {
    l->seq       = get_u32(&p[0]);
    l->op_type   = p[4];
    l->result    = p[5];
    l->equip_id  = get_u16(&p[6]);
    l->user_card = get_u32(&p[8]);
    l->timestamp = get_u32(&p[12]);
    l->due_time  = get_u32(&p[16]);
}

static void pack_config(uint8_t *p, const sys_config_t *c) {
    copy_str((char *)&p[0],  12U, c->device_id);
    copy_str((char *)&p[12], 24U, c->wifi_ssid);
    copy_str((char *)&p[36], 24U, c->wifi_pass);
    copy_str((char *)&p[60], 16U, c->server_host);
    put_u16(&p[76], c->server_port);
    put_u16(&p[78], c->overdue_days);
}

static void unpack_config(const uint8_t *p, sys_config_t *c) {
    copy_str(c->device_id,   (uint16_t)sizeof(c->device_id),   (const char *)&p[0]);
    copy_str(c->wifi_ssid,   (uint16_t)sizeof(c->wifi_ssid),   (const char *)&p[12]);
    copy_str(c->wifi_pass,   (uint16_t)sizeof(c->wifi_pass),   (const char *)&p[36]);
    copy_str(c->server_host, (uint16_t)sizeof(c->server_host), (const char *)&p[60]);
    c->server_port  = get_u16(&p[76]);
    c->overdue_days = get_u16(&p[78]);
}

/* ---------------------------------------------------------------- 对外接口 */

void drv_storage_init(void) {
    bsp_at24c256_init();
}

uint8_t drv_storage_selftest(void) {
    uint8_t wr[STORAGE_TEST_LEN];
    uint8_t rd[STORAGE_TEST_LEN];
    uint8_t i;

    if (!bsp_at24c256_probe()) {
        return 0U;
    }

    for (i = 0U; i < STORAGE_TEST_LEN; i++) {
        wr[i] = (uint8_t)(0xA5U ^ i);
    }
    if (!bsp_at24c256_write(STORAGE_TEST_ADDR, wr, STORAGE_TEST_LEN)) {
        return 0U;
    }
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

uint16_t drv_storage_boot_count(void) {
    uint8_t  buf[4];
    uint16_t count;

    if (!bsp_at24c256_read(STORAGE_BOOT_ADDR, buf, 4U)) {
        return 0U;
    }
    if ((buf[0] != STORAGE_BOOT_MAGIC0) || (buf[1] != STORAGE_BOOT_MAGIC1)) {
        count = 0U;
    } else {
        count = get_u16(&buf[2]);
        if (count >= 60000U) {
            count = 0U;
        }
    }
    count++;

    buf[0] = STORAGE_BOOT_MAGIC0;
    buf[1] = STORAGE_BOOT_MAGIC1;
    put_u16(&buf[2], count);
    if (!bsp_at24c256_write(STORAGE_BOOT_ADDR, buf, 4U)) {
        return 0U;
    }
    return count;
}

uint8_t drv_storage_load_config(sys_config_t *cfg) {
    uint8_t  buf[CFG_SIZE];
    uint16_t len = 0U, seq = 0U;
    uint8_t  use_b = 0U;

    if (cfg == 0) {
        return 0U;
    }
    if (!block_pick(ADDR_CONFIG_A, ADDR_CONFIG_B, buf, CFG_SIZE,
                    &len, &seq, &use_b)) {
        return 0U;
    }
    if (len != CFG_SIZE) {
        return 0U;
    }
    unpack_config(buf, cfg);
    return 1U;
}

void drv_storage_save_config(const sys_config_t *cfg) {
    uint8_t buf[CFG_SIZE];

    if (cfg == 0) {
        return;
    }
    pack_config(buf, cfg);
    (void)block_save(ADDR_CONFIG_A, ADDR_CONFIG_B, buf, CFG_SIZE);
}

uint16_t drv_storage_load_users(user_card_t *out, uint16_t max) {
    uint8_t  buf[STORAGE_MAX_USERS * REC_USER_SIZE];
    uint16_t len = 0U, seq = 0U;
    uint8_t  use_b = 0U;
    uint16_t n;
    uint16_t i;

    if ((out == 0) || (max == 0U)) {
        return 0U;
    }
    if (!block_pick(ADDR_USERS_A, ADDR_USERS_B, buf, (uint16_t)sizeof(buf),
                    &len, &seq, &use_b)) {
        return 0U;
    }

    n = (uint16_t)(len / REC_USER_SIZE);
    if (n > max) {
        n = max;
    }
    for (i = 0U; i < n; i++) {
        unpack_user(&buf[i * REC_USER_SIZE], &out[i]);
    }
    return n;
}

uint8_t drv_storage_save_users(const user_card_t *list, uint16_t count) {
    uint8_t buf[STORAGE_MAX_USERS * REC_USER_SIZE];
    uint16_t i;

    if ((list == 0) || (count > STORAGE_MAX_USERS)) {
        return 0U;
    }

    for (i = 0U; i < count; i++) {
        pack_user(&buf[i * REC_USER_SIZE], &list[i]);
    }
    /* count 为 0 时也要写一个最小长度，避免整块为空导致下次被判为无效 */
    if (count == 0U) {
        buf[0] = 0U;
    }
    return block_save(ADDR_USERS_A, ADDR_USERS_B, buf,
                      (uint16_t)((count == 0U) ? 1U : (count * REC_USER_SIZE)));
}

uint16_t drv_storage_load_equipment(equip_record_t *out, uint16_t max) {
    uint8_t  buf[STORAGE_MAX_EQUIP * REC_EQUIP_SIZE];
    uint16_t len = 0U, seq = 0U;
    uint8_t  use_b = 0U;
    uint16_t n;
    uint16_t i;

    if ((out == 0) || (max == 0U)) {
        return 0U;
    }
    if (!block_pick(ADDR_EQUIP_A, ADDR_EQUIP_B, buf, (uint16_t)sizeof(buf),
                    &len, &seq, &use_b)) {
        return 0U;
    }

    n = (uint16_t)(len / REC_EQUIP_SIZE);
    if (n > max) {
        n = max;
    }
    for (i = 0U; i < n; i++) {
        unpack_equip(&buf[i * REC_EQUIP_SIZE], &out[i]);
    }
    return n;
}

uint8_t drv_storage_save_equipment(const equip_record_t *list, uint16_t count) {
    uint8_t buf[STORAGE_MAX_EQUIP * REC_EQUIP_SIZE];
    uint16_t i;

    if ((list == 0) || (count > STORAGE_MAX_EQUIP)) {
        return 0U;
    }

    for (i = 0U; i < count; i++) {
        pack_equip(&buf[i * REC_EQUIP_SIZE], &list[i]);
    }
    if (count == 0U) {
        buf[0] = 0U;
    }
    return block_save(ADDR_EQUIP_A, ADDR_EQUIP_B, buf,
                      (uint16_t)((count == 0U) ? 1U : (count * REC_EQUIP_SIZE)));
}

/* ------------------------------------------------------------ 日志环形队列 */

/* 日志头：魔数(2) + 写入位置 head(2) + 有效条数 count(2) + 已上传条数 ack(2) + CRC(2)
 * head 是一直累加的写入计数，槽位 = head % LOG_SLOTS。
 * 有效记录为 [head-count, head-1] 这一段（按槽位取模）。 */
uint8_t drv_storage_append_log(const log_entry_t *e) {
    uint8_t  hdr[LOG_HDR_SIZE];
    uint8_t  rec[REC_LOG_SIZE];
    uint16_t head  = 0U;
    uint16_t count = 0U;
    uint16_t ack   = 0U;
    uint16_t slot;

    if (e == 0) {
        return 0U;
    }

    if (!bsp_at24c256_read(ADDR_LOGS, hdr, LOG_HDR_SIZE) ||
        (hdr[0] != STOR_MAGIC0) || (hdr[1] != STOR_MAGIC1) ||
        (proto_crc16(hdr, 8U) != get_u16(&hdr[8]))) {
        head = 0U;
        count = 0U;
        ack = 0U;
    } else {
        head  = get_u16(&hdr[2]);
        count = get_u16(&hdr[4]);
        ack   = get_u16(&hdr[6]);
        if (count > LOG_SLOTS) {
            count = LOG_SLOTS;
        }
        if (ack > count) {
            ack = count;
        }
    }

    slot = (uint16_t)(head % LOG_SLOTS);

    /* 先写记录体，最后更新头部。中途断电时头部还是旧的，
     * 最多丢掉这一条，不会破坏已有记录。 */
    pack_log(rec, e);
    if (!bsp_at24c256_write((uint16_t)(ADDR_LOGS + LOG_HDR_SIZE +
                                       slot * REC_LOG_SIZE), rec, REC_LOG_SIZE)) {
        return 0U;
    }

    head++;
    if (count < LOG_SLOTS) {
        count++;
    } else if (ack > 0U) {
        ack--;                     /* 环形覆盖掉最旧一条，已上传计数相应减一 */
    }

    hdr[0] = STOR_MAGIC0;
    hdr[1] = STOR_MAGIC1;
    put_u16(&hdr[2], head);
    put_u16(&hdr[4], count);
    put_u16(&hdr[6], ack);
    put_u16(&hdr[8], proto_crc16(hdr, 8U));

    return bsp_at24c256_write(ADDR_LOGS, hdr, LOG_HDR_SIZE);
}

uint16_t drv_storage_load_logs(log_entry_t *out, uint16_t max) {
    uint8_t  hdr[LOG_HDR_SIZE];
    uint8_t  rec[REC_LOG_SIZE];
    uint16_t head;
    uint16_t count;
    uint16_t n;
    uint16_t i;
    uint16_t slot;

    if ((out == 0) || (max == 0U)) {
        return 0U;
    }
    if (!bsp_at24c256_read(ADDR_LOGS, hdr, LOG_HDR_SIZE) ||
        (hdr[0] != STOR_MAGIC0) || (hdr[1] != STOR_MAGIC1) ||
        (proto_crc16(hdr, 8U) != get_u16(&hdr[8]))) {
        return 0U;
    }

    head  = get_u16(&hdr[2]);
    count = get_u16(&hdr[4]);
    if (count > LOG_SLOTS) {
        count = LOG_SLOTS;
    }

    n = (count < max) ? count : max;

    /* 按从旧到新的顺序读取 */
    for (i = 0U; i < n; i++) {
        slot = (uint16_t)((head - count + i) % LOG_SLOTS);
        if (!bsp_at24c256_read((uint16_t)(ADDR_LOGS + LOG_HDR_SIZE +
                                          slot * REC_LOG_SIZE), rec, REC_LOG_SIZE)) {
            return i;
        }
        unpack_log(rec, &out[i]);
    }
    return n;
}

void drv_storage_ack_log(void) {
    uint8_t  hdr[LOG_HDR_SIZE];
    uint16_t count;
    uint16_t ack;

    if (!bsp_at24c256_read(ADDR_LOGS, hdr, LOG_HDR_SIZE) ||
        (hdr[0] != STOR_MAGIC0) || (hdr[1] != STOR_MAGIC1) ||
        (proto_crc16(hdr, 8U) != get_u16(&hdr[8]))) {
        return;
    }

    count = get_u16(&hdr[4]);
    ack   = get_u16(&hdr[6]);
    if (ack >= count) {
        return;                        /* 没有待确认的记录 */
    }

    put_u16(&hdr[6], (uint16_t)(ack + 1U));
    put_u16(&hdr[8], proto_crc16(hdr, 8U));
    (void)bsp_at24c256_write(ADDR_LOGS, hdr, LOG_HDR_SIZE);
}

void drv_storage_poll(void) {
    /* 当前所有保存操作都是同步完成的，没有待处理的异步任务。 */
}

uint8_t drv_storage_flush(void) {
    /* 同上：数据在 save/append 返回时已经落盘 */
    return 1U;
}
