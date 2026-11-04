#include "bsp_spi.h"
#include "board_config.h"
#include "delay.h"

/* ============================================================================
 * 软件模拟 SPI（用于 RC522）
 *
 * 为什么不用 STM32 的硬件 SPI 外设：
 *   RC522 官方 STM32 例程用的是 GPIO 手工拉时序，特征是
 *     · SCK 空闲为高电平
 *     · 数据在 SCK 拉低之后采样（即下降沿方向采样）
 *   这与硬件 SPI 的 Mode 0（空闲低、上升沿采样）不一致。
 *   实测硬件 SPI 能正常读版本号，但收发命令失败，
 *   改用与官方一致的软件时序后即可正常工作。
 *
 * 引脚：SCK=PA5  MOSI=PA7  MISO=PA6  NSS=PA4（见 board_config.h）
 * ============================================================================ */

/* 每个电平保持的时间。官方例程用 200us（极慢但绝对可靠），
 * 实测降到 20us（约 25kHz）仍然稳定，寻卡一次约 50ms，
 * 主循环响应正常。若换模块后出现不稳定，可逐步调回 50/100us。 */
#define SPI_DELAY_US   20U

static void spi_delay(void) {
    delay_us(SPI_DELAY_US);
}

static void sck_set(uint8_t level) {
    if (level) {
        GPIO_SetBits(RC522_SCK_PORT, RC522_SCK_PIN);
    } else {
        GPIO_ResetBits(RC522_SCK_PORT, RC522_SCK_PIN);
    }
}

static void mosi_set(uint8_t level) {
    if (level) {
        GPIO_SetBits(RC522_MOSI_PORT, RC522_MOSI_PIN);
    } else {
        GPIO_ResetBits(RC522_MOSI_PORT, RC522_MOSI_PIN);
    }
}

static uint8_t miso_get(void) {
    return (GPIO_ReadInputDataBit(RC522_MISO_PORT, RC522_MISO_PIN) == Bit_SET)
           ? 1U : 0U;
}

void bsp_spi_init(void) {
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    /* SCK / MOSI / NSS：推挽输出（手工产生时序，不用复用功能） */
    gpio.GPIO_Pin   = RC522_SCK_PIN | RC522_MOSI_PIN | RC522_NSS_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(RC522_SCK_PORT, &gpio);

    /* MISO：浮空输入 */
    gpio.GPIO_Pin  = RC522_MISO_PIN;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(RC522_MISO_PORT, &gpio);

    /* 空闲状态：SCK 高、MOSI 低、CS 高（不选中） */
    sck_set(1U);
    mosi_set(0U);
    GPIO_SetBits(RC522_NSS_PORT, RC522_NSS_PIN);
}

uint8_t bsp_spi_transfer(uint8_t data) {
    uint8_t i;
    uint8_t result = 0U;

    for (i = 0U; i < 8U; i++) {
        /* 1. 先摆好 MOSI（高位在前） */
        mosi_set((data & 0x80U) ? 1U : 0U);
        data = (uint8_t)(data << 1);
        spi_delay();

        /* 2. SCK 拉低（下降沿） */
        sck_set(0U);
        spi_delay();

        /* 3. SCK 低电平期间采样 MISO —— 与官方例程采样点一致 */
        result = (uint8_t)(result << 1);
        if (miso_get() != 0U) {
            result |= 0x01U;
        }
        spi_delay();

        /* 4. SCK 拉高（上升沿），进入下一位 */
        sck_set(1U);
        spi_delay();
    }

    return result;
}

void bsp_spi_cs(uint8_t level) {
    if (level) {
        GPIO_SetBits(RC522_NSS_PORT, RC522_NSS_PIN);
    } else {
        GPIO_ResetBits(RC522_NSS_PORT, RC522_NSS_PIN);
    }
}
