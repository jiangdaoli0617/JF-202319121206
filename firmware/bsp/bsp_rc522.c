#include "bsp_rc522.h"
#include "bsp_spi.h"
#include "board_config.h"
#include "delay.h"

/* ============================================================================
 * MFRC522 读卡驱动（13.56MHz，MIFARE 卡）
 *
 * 读一张卡要经过四步，这是 ISO14443-A 协议规定的流程：
 *   1. REQA        —— 寻卡：问一句「附近有卡吗」，有卡会回应 ATQA
 *   2. Anticollision —— 防冲突：多张卡同时应答时，用 BCC 校验挑出一张
 *   3. Select      —— 选中：把这张卡的 ID 正式选上，卡回 SAK
 *   4. Halt        —— 挂起：告诉卡「先歇着」，下次寻卡不会重复读到同一张
 *
 * 寄存器访问方式（SPI 模式）：
 *   地址字节 = (寄存器地址 << 1) | 读写位
 *   写：最高位 0    读：最高位 1
 *   片选 NSS 低电平有效，每次读写都要单独拉低再拉高。
 * ============================================================================ */

/* ---------- 用到的寄存器 ---------- */
#define RC522_REG_COMMAND        0x01U
#define RC522_REG_COM_IRQ        0x04U
#define RC522_REG_ERROR          0x06U
#define RC522_REG_FIFO_DATA      0x09U
#define RC522_REG_FIFO_LEVEL     0x0AU
#define RC522_REG_CONTROL        0x0CU
#define RC522_REG_BIT_FRAMING    0x0DU
#define RC522_REG_COLL           0x0EU
#define RC522_REG_MODE           0x11U
#define RC522_REG_TX_MODE        0x12U
#define RC522_REG_RX_MODE        0x13U
#define RC522_REG_TX_CONTROL     0x14U
#define RC522_REG_TX_ASK         0x15U
#define RC522_REG_RF_CFG         0x26U
#define RC522_REG_T_MODE         0x2AU
#define RC522_REG_T_PRESCALER    0x2BU
#define RC522_REG_T_RELOAD_H     0x2CU
#define RC522_REG_T_RELOAD_L     0x2DU
#define RC522_REG_VERSION        0x37U

/* ---------- 命令 ---------- */
#define RC522_CMD_IDLE           0x00U
#define RC522_CMD_TRANSCEIVE     0x0CU
#define RC522_CMD_SOFT_RESET     0x0FU

/* ---------- PICC 指令（发给卡片的命令） ---------- */
#define PICC_REQA                0x26U
#define PICC_HALT                0x50U
#define PICC_ANTICOLL_CL1        0x93U
#define PICC_ANTICOLL_CL1_NVB    0x20U
#define PICC_SELECT_CL1          0x93U
#define PICC_SELECT_CL1_NVB      0x70U

/* ---------- 中断标志位（ComIrqReg） ---------- */
#define RC522_IRQ_TIMER          0x01U
#define RC522_IRQ_RX             0x20U
#define RC522_IRQ_IDLE           0x10U

/* 收发一次最多轮询这么多次寄存器；按 SPI 速度估算约 20ms 量级，
 * 超时就当「无卡」处理，绝不能让主循环卡死在这里。 */
#define RC522_POLL_LIMIT         8000U

/* ---------------------------- 寄存器读写 ---------------------------- */

static void rc522_write_reg(uint8_t reg, uint8_t value) {
    bsp_spi_cs(0);
    bsp_spi_transfer((uint8_t)((reg << 1) & 0x7EU));   /* 写：最高位为 0 */
    bsp_spi_transfer(value);
    bsp_spi_cs(1);
}

static uint8_t rc522_read_reg(uint8_t reg) {
    uint8_t value;

    bsp_spi_cs(0);
    bsp_spi_transfer((uint8_t)(((reg << 1) & 0x7EU) | 0x80U));  /* 读：最高位为 1 */
    value = bsp_spi_transfer(0x00U);
    bsp_spi_cs(1);
    return value;
}

/* 把寄存器某些位置 1 */
static void rc522_set_bits(uint8_t reg, uint8_t mask) {
    rc522_write_reg(reg, (uint8_t)(rc522_read_reg(reg) | mask));
}

/* 把寄存器某些位清 0 */
static void rc522_clear_bits(uint8_t reg, uint8_t mask) {
    rc522_write_reg(reg, (uint8_t)(rc522_read_reg(reg) & (uint8_t)~mask));
}

/* ---------------------------- 天线控制 ---------------------------- */

void bsp_rc522_antenna(uint8_t on) {
    if (on) {
        /* TxControlReg 的低两位分别控制 TX1 / TX2，都置 1 才发射 */
        rc522_set_bits(RC522_REG_TX_CONTROL, 0x03U);
    } else {
        rc522_clear_bits(RC522_REG_TX_CONTROL, 0x03U);
    }
}

/* ---------------------------- 初始化 ---------------------------- */

void bsp_rc522_init(void) {
    GPIO_InitTypeDef gpio;

    /* RST 引脚：推挽输出，低电平复位 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    gpio.GPIO_Pin   = RC522_RST_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(RC522_RST_PORT, &gpio);

    bsp_spi_cs(1);

    /* 硬复位：拉低 RST 再放开 */
    GPIO_ResetBits(RC522_RST_PORT, RC522_RST_PIN);
    delay_ms(10U);
    GPIO_SetBits(RC522_RST_PORT, RC522_RST_PIN);
    delay_ms(50U);

    /* 软复位 */
    rc522_write_reg(RC522_REG_COMMAND, RC522_CMD_SOFT_RESET);
    delay_ms(50U);

    /* 定时器：TAuto=1，收发结束自动启动定时器；30 个节拍约 10ms 超时 */
    rc522_write_reg(RC522_REG_T_MODE,      0x8DU);
    rc522_write_reg(RC522_REG_T_PRESCALER, 0x3EU);
    rc522_write_reg(RC522_REG_T_RELOAD_H,  0x00U);
    rc522_write_reg(RC522_REG_T_RELOAD_L,  0x1EU);

    /* 100% ASK 调制，MIFARE 卡要求 */
    rc522_write_reg(RC522_REG_TX_ASK, 0x40U);

    /* CRC 初值 0x6363 */
    rc522_write_reg(RC522_REG_MODE, 0x3DU);

    /* 接收增益 48dB（0x07 << 4），读卡距离更远 */
    rc522_write_reg(RC522_REG_RF_CFG, 0x70U);

    /* 106 kbit/s 速率 */
    rc522_write_reg(RC522_REG_TX_MODE, 0x00U);
    rc522_write_reg(RC522_REG_RX_MODE, 0x00U);

    /* 打开天线 */
    rc522_write_reg(RC522_REG_TX_CONTROL, 0x03U);
}

uint8_t bsp_rc522_version(void) {
    return rc522_read_reg(RC522_REG_VERSION);
}

/* ---------------------------- 收发核心 ---------------------------- */

/* 与卡片交换一帧数据。
 *   send / send_len    ：要发出去的字节
 *   back / back_len    ：接收缓冲；返回时 back_len 写成实际收到的字节数
 *   tx_last_bits       ：最后一个字节发送几位，整字节发传 0，REQA 传 7
 * 返回 1 表示成功，0 表示超时 / 出错。 */
static uint8_t rc522_transceive(const uint8_t *send, uint8_t send_len,
                                uint8_t *back, uint16_t *back_len,
                                uint8_t tx_last_bits) {
    uint16_t i;
    uint16_t poll;
    uint8_t  irq;
    uint8_t  error;
    uint8_t  fifo_level;

    /* 1. 让芯片回到空闲，清中断标志、清空 FIFO */
    rc522_write_reg(RC522_REG_COMMAND, RC522_CMD_IDLE);
    rc522_write_reg(RC522_REG_COM_IRQ, 0x7FU);
    rc522_write_reg(RC522_REG_FIFO_LEVEL, 0x80U);

    /* 2. 把要发的数据塞进 FIFO */
    for (i = 0U; i < send_len; i++) {
        rc522_write_reg(RC522_REG_FIFO_DATA, send[i]);
    }

    /* 3. 启动收发：先下命令，再置 StartSend 开始发送 */
    rc522_write_reg(RC522_REG_COMMAND, RC522_CMD_TRANSCEIVE);
    rc522_write_reg(RC522_REG_BIT_FRAMING,
                    (uint8_t)((tx_last_bits & 0x07U) | 0x80U));

    /* 4. 等中断：收到数据(RxIRq) / 传输完成(IdleIRq) / 超时(TimerIRq) */
    poll = 0U;
    do {
        irq = rc522_read_reg(RC522_REG_COM_IRQ);
        poll++;
    } while (((irq & (RC522_IRQ_RX | RC522_IRQ_IDLE)) == 0U) &&
             ((irq & RC522_IRQ_TIMER) == 0U) &&
             (poll < RC522_POLL_LIMIT));

    rc522_write_reg(RC522_REG_BIT_FRAMING, 0x00U);

    /* 超时，或者根本没有收到数据 */
    if (poll >= RC522_POLL_LIMIT) {
        return 0U;
    }
    if ((irq & (RC522_IRQ_RX | RC522_IRQ_IDLE)) == 0U) {
        return 0U;
    }

    /* 5. 检查错误位：BufferOvfl / ParityErr / ProtocolErr 任一置位都算失败 */
    error = rc522_read_reg(RC522_REG_ERROR);
    if ((error & 0x13U) != 0U) {
        return 0U;
    }

    /* 6. 取回接收到的数据 */
    fifo_level = rc522_read_reg(RC522_REG_FIFO_LEVEL);

    if (back != 0) {
        if (fifo_level > *back_len) {
            fifo_level = (uint8_t)*back_len;
        }
        for (i = 0U; i < fifo_level; i++) {
            back[i] = rc522_read_reg(RC522_REG_FIFO_DATA);
        }
        *back_len = fifo_level;
    } else {
        /* 调用方只关心「有没有回应」，不需要数据：把 FIFO 读空即可 */
        for (i = 0U; i < fifo_level; i++) {
            (void)rc522_read_reg(RC522_REG_FIFO_DATA);
        }
        *back_len = fifo_level;
    }

    return 1U;
}

/* ---------------------------- 读卡流程 ---------------------------- */

/* 第 1 步：寻卡。附近有卡时卡片会回应 2 字节 ATQA。 */
static uint8_t rc522_request(void) {
    uint8_t  cmd = PICC_REQA;
    uint8_t  back[2];
    uint16_t back_len = (uint16_t)sizeof(back);

    rc522_clear_bits(RC522_REG_COLL, 0x80U);        /* 清除「冲突已解决」标志 */

    /* 只发 7 位，因为 REQA 是短帧 */
    if (!rc522_transceive(&cmd, 1U, back, &back_len, 7U)) {
        return 0U;
    }
    return (back_len == 2U) ? 1U : 0U;
}

/* 第 2 步：防冲突。返回 4 字节 UID 并写入 uid（大端拼接）。 */
static uint8_t rc522_anticoll(uint32_t *uid) {
    uint8_t  send[2];
    uint8_t  back[5];
    uint16_t back_len = (uint16_t)sizeof(back);
    uint8_t  bcc;

    send[0] = PICC_ANTICOLL_CL1;
    send[1] = PICC_ANTICOLL_CL1_NVB;

    rc522_write_reg(RC522_REG_BIT_FRAMING, 0x00U);

    if (!rc522_transceive(send, 2U, back, &back_len, 0U)) {
        return 0U;
    }
    if (back_len != 5U) {
        return 0U;
    }

    /* 第 5 个字节是 BCC，等于前 4 字节异或；不等说明传输出错 */
    bcc = (uint8_t)(back[0] ^ back[1] ^ back[2] ^ back[3]);
    if (bcc != back[4]) {
        return 0U;
    }

    if (uid != 0) {
        *uid = ((uint32_t)back[0] << 24) | ((uint32_t)back[1] << 16) |
               ((uint32_t)back[2] << 8)  |  (uint32_t)back[3];
    }
    return 1U;
}

/* 第 3 步：选中卡片。卡回 SAK，表示已经进入「已选中」状态。 */
static uint8_t rc522_select(uint32_t uid) {
    uint8_t  send[7];
    uint8_t  back[3];
    uint16_t back_len = (uint16_t)sizeof(back);

    send[0] = PICC_SELECT_CL1;
    send[1] = PICC_SELECT_CL1_NVB;
    send[2] = (uint8_t)(uid >> 24);
    send[3] = (uint8_t)(uid >> 16);
    send[4] = (uint8_t)(uid >> 8);
    send[5] = (uint8_t)(uid);
    send[6] = (uint8_t)(send[2] ^ send[3] ^ send[4] ^ send[5]);

    /* 7 个字节：0x93 + 0x70 + 4 字节 UID + BCC（BCC 是 UID 四字节的异或） */
    if (!rc522_transceive(send, 7U, back, &back_len, 0U)) {
        return 0U;
    }
    return (back_len >= 1U) ? 1U : 0U;
}

/* 第 4 步：把卡挂起，避免下一轮重复读到同一张卡 */
static void rc522_halt(void) {
    uint8_t  send[4];
    uint16_t back_len = 0U;

    send[0] = PICC_HALT;
    send[1] = 0x00U;
    send[2] = 0x00U;
    send[3] = 0x00U;

    (void)rc522_transceive(send, 4U, 0, &back_len, 0U);
}

uint8_t bsp_rc522_read_uid(uint32_t *uid) {
    uint32_t id = 0U;

    if (!rc522_request()) {          /* 没有卡靠近，直接返回，不浪费时间 */
        return 0U;
    }
    if (!rc522_anticoll(&id)) {
        return 0U;
    }
    if (!rc522_select(id)) {
        return 0U;
    }
    rc522_halt();                    /* 读到了才挂起 */

    if (uid != 0) {
        *uid = id;
    }
    return 1U;
}
