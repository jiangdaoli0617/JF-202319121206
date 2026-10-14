#ifndef __BOARD_CONFIG_H
#define __BOARD_CONFIG_H

/**
  ******************************************************************************
  * @file    board_config.h
  * @brief   硬件配置总表 —— 本项目【所有引脚定义集中在这里】
  * @note    改硬件接线时，只需要改这一个文件，其它 .c 代码一律不用动！
  *          各驱动 .c 文件里禁止直接写 "GPIOA"、"GPIO_Pin_5" 这类硬编码，
  *          必须使用本文件中的宏。
  ******************************************************************************
  */

#include "stm32f10x.h"


/* ==========================================================================
 *                              一、系统参数
 * ========================================================================== */

/** 外部晶振频率：蓝色小板(STM32F103C8T6)板载 8MHz 晶振。
 *  如果你的板子焊的是 12MHz 晶振，把这里改成 12000000 即可。 */
#define BOARD_HSE_VALUE         8000000UL

/** 系统主频：8MHz * 9(PLL倍频) = 72MHz，F103 的最高主频。
 *  注意：软件延时函数 Delay_Us/Delay_Ms 依赖这个值，改错会导致延时不准。 */
#define BOARD_SYSCLK_HZ         72000000UL

/** 调试串口(Debug_USART)的开关：1=打开 printf 打印，0=关闭(节省Flash) */
#define DEBUG_PRINT_ENABLE      1


/* ==========================================================================
 *                          二、板载 LED（PC13）
 * ========================================================================== */
/* 蓝色小板 PC13 上焊了一颗 LED，一端接 3.3V，另一端接 PC13，
 * 所以 PC13 输出【低电平】时 LED 点亮，输出高电平时熄灭。 */
#define LED_PORT                GPIOC
#define LED_PIN                 GPIO_Pin_13
#define LED_CLK                 RCC_APB2Periph_GPIOC
#define LED_ON_LEVEL            0       /* 点亮电平：0=低电平点亮 */
#define LED_OFF_LEVEL           1       /* 熄灭电平 */


/* ==========================================================================
 *                       三、有源蜂鸣器（PB8）
 * ========================================================================== */
/* 有源蜂鸣器通电即响，不需要 PWM。但工作电流约 20~30mA，超过单个 IO 的
 * 安全输出电流(约 20mA)，因此用一颗 S8050 NPN 三极管做驱动：
 *      PB8 --[1kΩ]-- 基极；发射极接地；集电极接蜂鸣器负极；蜂鸣器正极接 3.3V/5V
 * 这样 PB8 输出【高电平】时三极管导通，蜂鸣器响。 */
#define BEEP_PORT               GPIOB
#define BEEP_PIN                GPIO_Pin_8
#define BEEP_CLK                RCC_APB2Periph_GPIOB
#define BEEP_ON_LEVEL           1       /* 响起电平：1=高电平响 */
#define BEEP_OFF_LEVEL          0       /* 停止电平 */


/* ==========================================================================
 *                        四、独立按键（5 个）
 * ========================================================================== */
/* 按键一端接引脚，另一端接地。引脚配置为【上拉输入】，
 * 所以：没按下 = 读到高电平(1)；按下 = 读到低电平(0)。 */
#define KEY_CLK                 RCC_APB2Periph_GPIOB

#define KEY_REG_PORT            GPIOB           /* 物品登记 */
#define KEY_REG_PIN             GPIO_Pin_11

#define KEY_OUT_PORT            GPIOB           /* 借出 */
#define KEY_OUT_PIN             GPIO_Pin_12

#define KEY_IN_PORT             GPIOB           /* 归还 */
#define KEY_IN_PIN              GPIO_Pin_13

#define KEY_PREV_PORT           GPIOB           /* 上一条记录 */
#define KEY_PREV_PIN            GPIO_Pin_14

#define KEY_NEXT_PORT           GPIOB           /* 下一条记录 */
#define KEY_NEXT_PIN            GPIO_Pin_15

#define KEY_PRESSED_LEVEL       0       /* 按下时的电平：0=按下为低电平 */


/* ==========================================================================
 *                 五、软件模拟 I2C 总线（OLED + AT24C02 + DS3231 共用）
 * ========================================================================== */
/* 三个器件挂在同一组 SCL/SDA 上，靠【器件地址】区分，互不冲突：
 *      OLED   = 0x78   AT24C02 = 0xA0   DS3231 = 0xD0
 * 之所以用软件模拟而不用 STM32 自带的硬件 I2C，是因为 F103 的硬件 I2C
 * 在标准库下容易出现总线死锁，软件模拟时序完全可控，也便于答辩讲解。 */
#define I2C_GPIO_CLK            RCC_APB2Periph_GPIOB

#define I2C_SCL_PORT            GPIOB
#define I2C_SCL_PIN             GPIO_Pin_6

#define I2C_SDA_PORT            GPIOB
#define I2C_SDA_PIN             GPIO_Pin_7

/* 三个从机器件地址（写方向，即最低位为 0 的 8 位地址） */
#define OLED_I2C_ADDR           0x78
#define AT24C02_I2C_ADDR        0xA0
#define DS3231_I2C_ADDR         0xD0


/* ==========================================================================
 *                     六、RC522 RFID 读卡模块（SPI1）
 * ========================================================================== */
/* RC522 使用 STM32 的硬件 SPI1 通信。SPI1 的引脚是固定的，不能随便改：
 *      PA5=SCK  PA6=MISO  PA7=MOSI   （这三个是 SPI1 的复用功能引脚）
 *      PA4 用作片选 CS（普通推挽输出即可）     PB0 用作复位 RST
 * 特别注意：RC522 必须用 3.3V 供电，千万不能接 5V！ */
#define RC522_SPI               SPI1
#define RC522_SPI_CLK           RCC_APB2Periph_SPI1

#define RC522_GPIO_CLK          RCC_APB2Periph_GPIOA
#define RC522_CS_PORT           GPIOA
#define RC522_CS_PIN            GPIO_Pin_4      /* 片选，低电平有效 */
#define RC522_SCK_PORT          GPIOA
#define RC522_SCK_PIN           GPIO_Pin_5
#define RC522_MISO_PORT         GPIOA
#define RC522_MISO_PIN          GPIO_Pin_6
#define RC522_MOSI_PORT         GPIOA
#define RC522_MOSI_PIN          GPIO_Pin_7

#define RC522_RST_CLK           RCC_APB2Periph_GPIOB
#define RC522_RST_PORT          GPIOB
#define RC522_RST_PIN           GPIO_Pin_0      /* 复位，低电平复位 */

/* RC522 SPI 时钟分频：72MHz/64 = 1.125MHz，
 * RC522 最高支持 10MHz，这个速度稳定可靠。 */
#define RC522_SPI_PRESCALER     SPI_BaudRatePrescaler_64


/* ==========================================================================
 *              七、ESP-01S WiFi 模块（USART1）—— 接 Java 上位机
 * ========================================================================== */
/* 交叉接线：STM32 的 TX 接 ESP 的 RX，STM32 的 RX 接 ESP 的 TX。
 * ESP-01S 出厂默认波特率 115200。
 * 【供电提醒】ESP-01S 发射瞬间电流可达 300mA，用开发板 3.3V 排针供电
 * 容易导致它反复重启（表现为 AT 指令无响应），建议单独用一路 AMS1117-3.3 供电。 */
#define ESP_USART               USART1
#define ESP_USART_CLK           RCC_APB2Periph_USART1
#define ESP_GPIO_CLK            RCC_APB2Periph_GPIOA
#define ESP_USART_BAUD          115200

#define ESP_TX_PORT             GPIOA
#define ESP_TX_PIN              GPIO_Pin_9      /* STM32 发送 → ESP 的 RX */
#define ESP_RX_PORT             GPIOA
#define ESP_RX_PIN              GPIO_Pin_10     /* STM32 接收 ← ESP 的 TX */


/* ==========================================================================
 *        八、调试串口（USART2）—— 接 USB-TTL，用 printf 看运行信息
 * ========================================================================== */
/* 这个串口不参与业务，纯粹用来在电脑上用串口助手看打印信息、方便排错。
 * 波特率 115200，8 位数据位，1 位停止位，无校验。 */
#define DEBUG_USART             USART2
#define DEBUG_USART_CLK         RCC_APB1Periph_USART2
#define DEBUG_GPIO_CLK          RCC_APB2Periph_GPIOA
#define DEBUG_USART_BAUD        115200

#define DEBUG_TX_PORT           GPIOA
#define DEBUG_TX_PIN            GPIO_Pin_2      /* STM32 发送 → USB-TTL 的 RX */
#define DEBUG_RX_PORT           GPIOA
#define DEBUG_RX_PIN            GPIO_Pin_3      /* STM32 接收 ← USB-TTL 的 TX */


#endif /* __BOARD_CONFIG_H */
