#include "bsp_i2c.h"
#include "board_config.h"
#include "delay.h"

/* ============================================================================
 * 软件模拟 I2C（开题报告确定的方案）
 *
 * SCL / SDA 均配置为【开漏输出】：
 *   输出 0  → 主动把总线拉低
 *   输出 1  → 释放总线，由外部上拉电阻把电平拉高
 * 开漏模式下引脚的输入通路依然有效，因此可以用 GPIO_ReadInputDataBit
 * 读回总线上的真实电平，用来接收从机的应答位和数据位。
 *
 * 一个完整 SCL 周期 = 2 * I2C_DELAY_US。
 * ============================================================================ */

/* 从机做时钟拉伸时会把 SCL 按住不放，最多等这么多微秒后强行继续，
 * 避免主循环死等在一个坏掉的总线上。 */
#define I2C_SCL_STRETCH_LOOPS   50U

static void i2c_delay(void) {
    delay_us(I2C_DELAY_US);
}

static void scl_low(void) {
    GPIO_ResetBits(I2C_SCL_PORT, I2C_SCL_PIN);
}

static void sda_low(void) {
    GPIO_ResetBits(I2C_SDA_PORT, I2C_SDA_PIN);
}

static void sda_high(void) {
    GPIO_SetBits(I2C_SDA_PORT, I2C_SDA_PIN);
}

static uint8_t sda_read(void) {
    return (GPIO_ReadInputDataBit(I2C_SDA_PORT, I2C_SDA_PIN) == Bit_SET) ? 1U : 0U;
}

/* 释放 SCL 并等它真的变高 */
static void scl_release(void) {
    uint8_t n = I2C_SCL_STRETCH_LOOPS;

    GPIO_SetBits(I2C_SCL_PORT, I2C_SCL_PIN);
    while (n-- > 0U) {
        if (GPIO_ReadInputDataBit(I2C_SCL_PORT, I2C_SCL_PIN) == Bit_SET) {
            return;
        }
        delay_us(1U);
    }
}

/* 起始位：SCL 保持高电平期间，SDA 由高变低 */
static void i2c_start(void) {
    sda_high();
    scl_release();
    i2c_delay();
    sda_low();
    i2c_delay();
    scl_low();
    i2c_delay();
}

/* 停止位：SCL 保持高电平期间，SDA 由低变高 */
static void i2c_stop(void) {
    scl_low();
    sda_low();
    i2c_delay();
    scl_release();
    i2c_delay();
    sda_high();
    i2c_delay();
}

/* 发送一个字节，返回 1 = 从机应答(ACK)，0 = 非应答(NACK) */
static uint8_t i2c_write_byte(uint8_t b) {
    uint8_t i;
    uint8_t ack;

    for (i = 0U; i < 8U; i++) {
        if ((b & 0x80U) != 0U) {
            sda_high();
        } else {
            sda_low();
        }
        b = (uint8_t)(b << 1);
        i2c_delay();
        scl_release();          /* SCL 高电平期间数据位必须保持稳定 */
        i2c_delay();
        scl_low();
        i2c_delay();
    }

    /* 第 9 个时钟：主机释放 SDA，由从机拉低表示应答 */
    sda_high();
    i2c_delay();
    scl_release();
    i2c_delay();
    ack = (sda_read() == 0U) ? 1U : 0U;
    scl_low();
    i2c_delay();

    return ack;
}

/* 接收一个字节；ack = 1 表示后面还要继续读，0 表示这是最后一个字节 */
static uint8_t i2c_read_byte(uint8_t ack) {
    uint8_t i;
    uint8_t b = 0U;

    sda_high();                 /* 释放 SDA，交给从机驱动 */
    for (i = 0U; i < 8U; i++) {
        i2c_delay();
        scl_release();
        i2c_delay();
        b = (uint8_t)((b << 1) | sda_read());
        scl_low();
        i2c_delay();
    }

    /* 第 9 个时钟：由主机给出应答 */
    if (ack != 0U) {
        sda_low();
    } else {
        sda_high();
    }
    i2c_delay();
    scl_release();
    i2c_delay();
    scl_low();
    sda_high();                 /* 读完立即释放总线 */
    i2c_delay();

    return b;
}

void bsp_i2c_init(void) {
    GPIO_InitTypeDef gpio;
    uint8_t i;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    gpio.GPIO_Pin   = I2C_SCL_PIN | I2C_SDA_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_Out_OD;      /* 开漏：高电平靠外部上拉 */
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(I2C_SCL_PORT, &gpio);

    /* 上电时从机可能卡在一次被打断的传输里把 SDA 拉到低电平。
     * 先补 9 个时钟让它把残留字节发完，再补一个停止位回到空闲态。 */
    sda_high();
    for (i = 0U; i < 9U; i++) {
        scl_low();
        i2c_delay();
        scl_release();
        i2c_delay();
    }
    i2c_stop();
}

uint8_t bsp_i2c_write(uint8_t addr, const uint8_t *data, uint16_t len) {
    uint16_t i;

    if (len == 0U) {
        return 0U;
    }

    i2c_start();
    if (!i2c_write_byte((uint8_t)(addr << 1))) {
        i2c_stop();
        return 0U;
    }
    for (i = 0U; i < len; i++) {
        if (!i2c_write_byte(data[i])) {
            i2c_stop();
            return 0U;
        }
    }
    i2c_stop();
    return 1U;
}

uint8_t bsp_i2c_read(uint8_t addr, uint8_t *data, uint16_t len) {
    uint16_t i;

    if (len == 0U) {
        return 0U;
    }

    i2c_start();
    if (!i2c_write_byte((uint8_t)((addr << 1) | 0x01U))) {
        i2c_stop();
        return 0U;
    }
    for (i = 0U; i < len; i++) {
        data[i] = i2c_read_byte((i == (uint16_t)(len - 1U)) ? 0U : 1U);
    }
    i2c_stop();
    return 1U;
}

uint8_t bsp_i2c_write_reg(uint8_t addr, uint8_t reg,
                          const uint8_t *data, uint16_t len) {
    uint16_t i;

    i2c_start();
    if (!i2c_write_byte((uint8_t)(addr << 1)) || !i2c_write_byte(reg)) {
        i2c_stop();
        return 0U;
    }
    for (i = 0U; i < len; i++) {
        if (!i2c_write_byte(data[i])) {
            i2c_stop();
            return 0U;
        }
    }
    i2c_stop();
    return 1U;
}

uint8_t bsp_i2c_read_reg(uint8_t addr, uint8_t reg,
                         uint8_t *data, uint16_t len) {
    uint16_t i;

    if (len == 0U) {
        return 0U;
    }

    /* 先写寄存器地址，再用重复起始位把方向切换为读 */
    i2c_start();
    if (!i2c_write_byte((uint8_t)(addr << 1)) || !i2c_write_byte(reg)) {
        i2c_stop();
        return 0U;
    }

    i2c_start();
    if (!i2c_write_byte((uint8_t)((addr << 1) | 0x01U))) {
        i2c_stop();
        return 0U;
    }
    for (i = 0U; i < len; i++) {
        data[i] = i2c_read_byte((i == (uint16_t)(len - 1U)) ? 0U : 1U);
    }
    i2c_stop();
    return 1U;
}

uint8_t bsp_i2c_scan(uint8_t *found, uint8_t max) {
    uint8_t addr;
    uint8_t count = 0U;

    /* 0x78 及以上是 10 位地址前缀和保留地址，普通 7 位器件只扫到这里 */
    for (addr = 0x01U; addr < 0x78U; addr++) {
        i2c_start();
        if (i2c_write_byte((uint8_t)(addr << 1)) && count < max) {
            found[count++] = addr;
        }
        i2c_stop();
        delay_ms(1U);
    }

    return count;
}
