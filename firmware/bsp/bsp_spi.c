#include "bsp_spi.h"
#include "board_config.h"

#define SPI_TIMEOUT 100000U

void bsp_spi_init(void) {
    GPIO_InitTypeDef gpio;
    SPI_InitTypeDef  spi;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA |
                           RCC_APB2Periph_SPI1, ENABLE);

    /* SCK / MOSI：复用推挽 */
    gpio.GPIO_Pin   = RC522_SCK_PIN | RC522_MOSI_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(RC522_SCK_PORT, &gpio);

    /* MISO：浮空输入 */
    gpio.GPIO_Pin  = RC522_MISO_PIN;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(RC522_MISO_PORT, &gpio);

    /* 软件片选 */
    gpio.GPIO_Pin   = RC522_NSS_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(RC522_NSS_PORT, &gpio);
    GPIO_SetBits(RC522_NSS_PORT, RC522_NSS_PIN);

    spi.SPI_Direction         = SPI_Direction_2Lines_FullDuplex;
    spi.SPI_Mode              = SPI_Mode_Master;
    spi.SPI_DataSize          = SPI_DataSize_8b;
    spi.SPI_CPOL              = SPI_CPOL_Low;
    spi.SPI_CPHA              = SPI_CPHA_1Edge;
    spi.SPI_NSS               = SPI_NSS_Soft;
    spi.SPI_BaudRatePrescaler = RC522_SPI_PRESCALER;
    spi.SPI_FirstBit          = SPI_FirstBit_MSB;
    spi.SPI_CRCPolynomial     = 7;
    SPI_Init(RC522_SPI, &spi);

    SPI_Cmd(RC522_SPI, ENABLE);
}

uint8_t bsp_spi_transfer(uint8_t data) {
    uint32_t timeout = SPI_TIMEOUT;

    while (SPI_I2S_GetFlagStatus(RC522_SPI, SPI_I2S_FLAG_TXE) == RESET) {
        if (--timeout == 0U) {
            return 0xFFU;
        }
    }
    SPI_I2S_SendData(RC522_SPI, data);

    timeout = SPI_TIMEOUT;
    while (SPI_I2S_GetFlagStatus(RC522_SPI, SPI_I2S_FLAG_RXNE) == RESET) {
        if (--timeout == 0U) {
            return 0xFFU;
        }
    }
    return (uint8_t)SPI_I2S_ReceiveData(RC522_SPI);
}

void bsp_spi_cs(uint8_t level) {
    if (level) {
        GPIO_SetBits(RC522_NSS_PORT, RC522_NSS_PIN);
    } else {
        GPIO_ResetBits(RC522_NSS_PORT, RC522_NSS_PIN);
    }
}
