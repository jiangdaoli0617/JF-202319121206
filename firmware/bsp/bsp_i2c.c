#include "bsp_i2c.h"
#include "board_config.h"
#include "delay.h"

#define I2C_TIMEOUT 100000U

static uint8_t wait_event(uint32_t event) {
    uint32_t timeout = I2C_TIMEOUT;

    while (I2C_CheckEvent(I2C_BUS, event) != SUCCESS) {
        if (--timeout == 0U) {
            I2C_GenerateSTOP(I2C_BUS, ENABLE);
            I2C_ClearFlag(I2C_BUS, I2C_FLAG_AF);
            return 0U;
        }
    }
    return 1U;
}

/* 发送一个字节，等待发送寄存器空 */
static uint8_t send_byte(uint8_t b) {
    if (!wait_event(I2C_EVENT_MASTER_BYTE_TRANSMITTING)) {
        return 0U;
    }
    I2C_SendData(I2C_BUS, b);
    return 1U;
}

/* 发起 START 并发送从机地址 */
static uint8_t start_with_addr(uint8_t addr, uint8_t direction) {
    I2C_GenerateSTART(I2C_BUS, ENABLE);
    if (!wait_event(I2C_EVENT_MASTER_MODE_SELECT)) {
        return 0U;
    }

    I2C_Send7bitAddress(I2C_BUS, (uint8_t)(addr << 1), direction);
    return wait_event(direction == I2C_Direction_Transmitter
                      ? I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED
                      : I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED);
}

void bsp_i2c_init(void) {
    GPIO_InitTypeDef gpio;
    I2C_InitTypeDef  i2c;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

    gpio.GPIO_Pin   = I2C_SCL_PIN | I2C_SDA_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_AF_OD;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(I2C_SCL_PORT, &gpio);

    i2c.I2C_Mode                = I2C_Mode_I2C;
    i2c.I2C_DutyCycle           = I2C_DutyCycle_2;
    i2c.I2C_OwnAddress1         = 0x00;
    i2c.I2C_Ack                 = I2C_Ack_Enable;
    i2c.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    i2c.I2C_ClockSpeed          = I2C_CLOCK_SPEED;
    I2C_Init(I2C_BUS, &i2c);

    I2C_Cmd(I2C_BUS, ENABLE);
}

uint8_t bsp_i2c_write(uint8_t addr, const uint8_t *data, uint16_t len) {
    uint16_t i;

    if (len == 0U) {
        return 0U;
    }
    if (!start_with_addr(addr, I2C_Direction_Transmitter)) {
        return 0U;
    }

    for (i = 0U; i < len; i++) {
        if (!send_byte(data[i])) {
            return 0U;
        }
    }

    if (!wait_event(I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
        return 0U;
    }
    I2C_GenerateSTOP(I2C_BUS, ENABLE);
    return 1U;
}

uint8_t bsp_i2c_read(uint8_t addr, uint8_t *data, uint16_t len) {
    uint16_t i;

    if (len == 0U) {
        return 0U;
    }
    if (!start_with_addr(addr, I2C_Direction_Receiver)) {
        return 0U;
    }

    for (i = 0U; i < len; i++) {
        if (i == (uint16_t)(len - 1U)) {
            I2C_AcknowledgeConfig(I2C_BUS, DISABLE);
        }
        if (!wait_event(I2C_EVENT_MASTER_BYTE_RECEIVED)) {
            I2C_AcknowledgeConfig(I2C_BUS, ENABLE);
            return 0U;
        }
        data[i] = I2C_ReceiveData(I2C_BUS);
    }

    I2C_GenerateSTOP(I2C_BUS, ENABLE);
    I2C_AcknowledgeConfig(I2C_BUS, ENABLE);
    return 1U;
}

uint8_t bsp_i2c_write_reg(uint8_t addr, uint8_t reg,
                          const uint8_t *data, uint16_t len) {
    uint16_t i;

    if (!start_with_addr(addr, I2C_Direction_Transmitter)) {
        return 0U;
    }
    if (!send_byte(reg)) {
        return 0U;
    }
    for (i = 0U; i < len; i++) {
        if (!send_byte(data[i])) {
            return 0U;
        }
    }

    if (!wait_event(I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
        return 0U;
    }
    I2C_GenerateSTOP(I2C_BUS, ENABLE);
    return 1U;
}

uint8_t bsp_i2c_read_reg(uint8_t addr, uint8_t reg,
                         uint8_t *data, uint16_t len) {
    uint16_t i;

    if (len == 0U) {
        return 0U;
    }

    /* 先写寄存器地址 */
    if (!start_with_addr(addr, I2C_Direction_Transmitter)) {
        return 0U;
    }
    if (!send_byte(reg)) {
        return 0U;
    }
    if (!wait_event(I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
        return 0U;
    }

    /* 重复 START，切换为接收 */
    I2C_GenerateSTART(I2C_BUS, ENABLE);
    if (!wait_event(I2C_EVENT_MASTER_MODE_SELECT)) {
        return 0U;
    }
    I2C_Send7bitAddress(I2C_BUS, (uint8_t)(addr << 1), I2C_Direction_Receiver);
    if (!wait_event(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)) {
        return 0U;
    }

    for (i = 0U; i < len; i++) {
        if (i == (uint16_t)(len - 1U)) {
            I2C_AcknowledgeConfig(I2C_BUS, DISABLE);
        }
        if (!wait_event(I2C_EVENT_MASTER_BYTE_RECEIVED)) {
            I2C_AcknowledgeConfig(I2C_BUS, ENABLE);
            return 0U;
        }
        data[i] = I2C_ReceiveData(I2C_BUS);
    }

    I2C_GenerateSTOP(I2C_BUS, ENABLE);
    I2C_AcknowledgeConfig(I2C_BUS, ENABLE);
    return 1U;
}

uint8_t bsp_i2c_scan(uint8_t *found, uint8_t max) {
    uint8_t  count = 0U;
    uint8_t  addr;
    uint32_t timeout;

    for (addr = 0x01U; addr < 0x80U; addr++) {
        I2C_GenerateSTART(I2C_BUS, ENABLE);

        timeout = I2C_TIMEOUT;
        while (I2C_CheckEvent(I2C_BUS, I2C_EVENT_MASTER_MODE_SELECT) != SUCCESS) {
            if (--timeout == 0U) {
                break;
            }
        }
        if (timeout == 0U) {
            I2C_GenerateSTOP(I2C_BUS, ENABLE);
            I2C_ClearFlag(I2C_BUS, I2C_FLAG_AF);
            continue;
        }

        I2C_Send7bitAddress(I2C_BUS, (uint8_t)(addr << 1),
                            I2C_Direction_Transmitter);

        timeout = I2C_TIMEOUT;
        while (I2C_CheckEvent(I2C_BUS,
               I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) != SUCCESS) {
            if (--timeout == 0U) {
                break;
            }
        }

        I2C_GenerateSTOP(I2C_BUS, ENABLE);

        if (timeout != 0U && count < max) {
            found[count++] = addr;
        }
        I2C_ClearFlag(I2C_BUS, I2C_FLAG_AF);
        delay_ms(1U);
    }

    return count;
}
