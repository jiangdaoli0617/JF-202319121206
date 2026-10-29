#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include "stm32f10x.h"

/* ============================================================
 * 板级引脚定义
 * 与 docs/03-硬件设计与接线.md 保持一致。
 * 改硬件只改这里，驱动层不做假设。
 * ============================================================ */

/* ---------- 系统滴答（stm32f10x_it.c 中定义） ---------- */
extern volatile uint32_t g_sys_tick_ms;

/* ---------- 调试串口 USART2：接 USB-TTL，看 printf 输出 ---------- */
#define DBG_USART             USART2
#define DBG_USART_BAUD        115200
#define DBG_TX_PORT           GPIOA
#define DBG_TX_PIN            GPIO_Pin_2
#define DBG_RX_PORT           GPIOA
#define DBG_RX_PIN            GPIO_Pin_3

/* ---------- WiFi 串口 USART1（ESP8266-01S） ---------- */
/* 交叉接线：STM32 的 TX 接 ESP 的 RX，STM32 的 RX 接 ESP 的 TX。
 * 注意：CH_PD 由外部 10k 上拉到 3.3V，不由 MCU 控制 */
#define WIFI_USART            USART1
#define WIFI_USART_BAUD       115200
#define WIFI_TX_PORT          GPIOA
#define WIFI_TX_PIN           GPIO_Pin_9
#define WIFI_RX_PORT          GPIOA
#define WIFI_RX_PIN           GPIO_Pin_10

/* ---------- RC522 SPI1 ---------- */
#define RC522_SPI             SPI1
#define RC522_SCK_PORT        GPIOA
#define RC522_SCK_PIN         GPIO_Pin_5
#define RC522_MISO_PORT       GPIOA
#define RC522_MISO_PIN        GPIO_Pin_6
#define RC522_MOSI_PORT       GPIOA
#define RC522_MOSI_PIN        GPIO_Pin_7
#define RC522_NSS_PORT        GPIOA
#define RC522_NSS_PIN         GPIO_Pin_4
#define RC522_RST_PORT        GPIOB
#define RC522_RST_PIN         GPIO_Pin_0

/* 72 MHz / 64 = 1.125 MHz，RC522 上限 10 MHz，留足余量 */
#define RC522_SPI_PRESCALER   SPI_BaudRatePrescaler_64

/* ---------- 软件模拟 I2C：DS3231 / AT24C02 / SSD1306 ---------- */
/* 三个器件挂在同一组 SCL/SDA 上，靠器件地址区分，互不冲突。
 * 不使用 F103 自带的硬件 I2C 外设：标准库下容易出现总线死锁、需要反复复位；
 * 软件模拟用普通 GPIO 加微秒延时手工产生时序，完全可控，也便于答辩讲原理。 */
#define I2C_SCL_PORT          GPIOB
#define I2C_SCL_PIN           GPIO_Pin_6
#define I2C_SDA_PORT          GPIOB
#define I2C_SDA_PIN           GPIO_Pin_7

/* 半周期延时(us)：5us → SCL 周期 10us → 约 100kHz，标准模式，排线较长时也稳。
 * 调小可提速，但不要低于 2us（对应约 250kHz）。 */
#define I2C_DELAY_US          5U

/* 器件地址用 7 位形式，底层发送时左移一位再补读写位；
 * 括号内是开题报告中写法的 8 位写地址，两者是同一器件。 */
#define DS3231_ADDR           0x68      /* 8 位写地址 0xD0 */
#define AT24C02_ADDR          0x50      /* 8 位写地址 0xA0 */
#define OLED_ADDR             0x3C      /* 8 位写地址 0x78 */

/* ---------- 按键 K1~K5 ---------- */
/* 上拉输入，按下接地；5 个键分别对应
 * 登记 / 借出 / 归还 / 上一条记录 / 下一条记录 */
#define KEY_PORT              GPIOB
#define KEY1_PIN              GPIO_Pin_11     /* 物品登记 */
#define KEY2_PIN              GPIO_Pin_12     /* 借出 */
#define KEY3_PIN              GPIO_Pin_13     /* 归还 */
#define KEY4_PIN              GPIO_Pin_14     /* 上一条记录 */
#define KEY5_PIN              GPIO_Pin_15     /* 下一条记录 */
#define KEY_COUNT             5

/* ---------- 蜂鸣器与 LED ---------- */
#define BEEP_PORT             GPIOB
#define BEEP_PIN              GPIO_Pin_8

/* 蜂鸣器触发极性：
 *   0 = 低电平鸣叫 —— 江协那套 3 脚有源蜂鸣器模块（板上自带三极管），
 *                     I/O 接 PB8，VCC 接 3.3V，GND 接 GND，不需要外接电阻
 *   1 = 高电平鸣叫 —— 自己用 S8050 三极管搭的驱动电路（基极串 1kΩ）
 * 接反了只会表现为「该响时不响、不响时一直响」，改这一个数字即可。 */
#define BEEP_ON_LEVEL         0

#define LED_GREEN_PORT        GPIOB
#define LED_GREEN_PIN         GPIO_Pin_9

#define LED_RED_PORT          GPIOC
#define LED_RED_PIN           GPIO_Pin_13

/* 0 = 低电平点亮，1 = 高电平点亮。
 * PC13 若为 Blue Pill 板载 LED，则为低电平点亮。 */
#define LED_GREEN_ACTIVE_HIGH 1
#define LED_RED_ACTIVE_HIGH   0

/* ============================================================
 * 时间参数
 * ============================================================ */
#define RTC_CACHE_MS          1000U     /* drv_rtc_now 的缓存刷新周期 */
#define RFID_DEDUP_MS         2000U     /* 同卡去重窗口 */
#define KEY_DEBOUNCE_MS       20U
#define OVERDUE_SCAN_SEC      60U       /* 超期扫描周期 */
#define OVERDUE_BEEP_SEC      1800U     /* 超期提醒间隔 30 min */

#endif /* BOARD_CONFIG_H */
