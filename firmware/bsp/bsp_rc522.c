#include "bsp_rc522.h"
#include "bsp_spi.h"
#include "board_config.h"
#include "delay.h"

/* RC522 常用寄存器 */
#define RC522_REG_COMMAND        0x01
#define RC522_REG_COM_IRQ        0x04
#define RC522_REG_FIFO_DATA      0x09
#define RC522_REG_FIFO_LEVEL     0x0A
#define RC522_REG_BIT_FRAMING    0x0D
#define RC522_REG_COLL           0x0E
#define RC522_REG_MODE           0x11
#define RC522_REG_TX_CONTROL     0x14
#define RC522_REG_TX_ASK         0x15
#define RC522_REG_VERSION        0x37

#define RC522_CMD_IDLE           0x00
#define RC522_CMD_TRANSCEIVE     0x0C
#define RC522_CMD_SOFT_RESET     0x0F

static void rc522_write_reg(uint8_t reg, uint8_t value) {
    bsp_spi_cs(0);
    bsp_spi_transfer((uint8_t)((reg << 1) & 0x7EU));
    bsp_spi_transfer(value);
    bsp_spi_cs(1);
}

static uint8_t rc522_read_reg(uint8_t reg) {
    uint8_t value;

    bsp_spi_cs(0);
    bsp_spi_transfer((uint8_t)(((reg << 1) & 0x7EU) | 0x80U));
    value = bsp_spi_transfer(0x00U);
    bsp_spi_cs(1);
    return value;
}

void bsp_rc522_init(void) {
    GPIO_InitTypeDef gpio;

    /* RST 引脚 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    gpio.GPIO_Pin   = RC522_RST_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(RC522_RST_PORT, &gpio);

    bsp_spi_cs(1);
    GPIO_ResetBits(RC522_RST_PORT, RC522_RST_PIN);
    delay_ms(10U);
    GPIO_SetBits(RC522_RST_PORT, RC522_RST_PIN);
    delay_ms(50U);

    rc522_write_reg(RC522_REG_COMMAND, RC522_CMD_SOFT_RESET);
    delay_ms(50U);

    /* TODO: 配置定时器、调制方式、天线增益 */
    bsp_rc522_antenna(1U);
}

uint8_t bsp_rc522_version(void) {
    return rc522_read_reg(RC522_REG_VERSION);
}

uint8_t bsp_rc522_read_uid(uint32_t *uid) {
    (void)uid;
    /* TODO: REQA → 防冲突 → Select → 读 UID */
    return 0U;
}

void bsp_rc522_antenna(uint8_t on) {
    uint8_t value = rc522_read_reg(RC522_REG_TX_CONTROL);

    if (on) {
        value |= 0x03U;
    } else {
        value &= (uint8_t)~0x03U;
    }
    rc522_write_reg(RC522_REG_TX_CONTROL, value);
}
