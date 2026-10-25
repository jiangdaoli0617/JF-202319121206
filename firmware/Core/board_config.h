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

/* ---------- 调试串口 USART1 ---------- */
#define DBG_USART             USART1
#define DBG_USART_BAUD        115200
#define DBG_TX_PORT           GPIOA
#define DBG_TX_PIN            GPIO_Pin_9
#define DBG_RX_PORT           GPIOA
#define DBG_RX_PIN            GPIO_Pin_10

/* ---------- WiFi 串口 USART2（ESP8266-01S） ---------- */
/* 注意：CH_PD 由外部 10k 上拉到 3.3V，不由 MCU 控制 */
#define WIFI_USART            USART2
#define WIFI_USART_BAUD       115200
#define WIFI_TX_PORT          GPIOA
#define WIFI_TX_PIN           GPIO_Pin_2
#define WIFI_RX_PORT          GPIOA
#define WIFI_RX_PIN           GPIO_Pin_3

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

/* ---------- I2C1：DS3231 / AT24C02 / SSD1306 ---------- */
#define I2C_BUS               I2C1
#define I2C_SCL_PORT          GPIOB
#define I2C_SCL_PIN           GPIO_Pin_6
#define I2C_SDA_PORT          GPIOB
#define I2C_SDA_PIN           GPIO_Pin_7

/* 400 kHz 为标准模式上限；排线较长或通信不稳时改为 100000 */
#define I2C_CLOCK_SPEED       400000

#define DS3231_ADDR           0x68
#define AT24C02_ADDR          0x50
#define OLED_ADDR             0x3C

/* ---------- 按键 K1~K4 ---------- */
#define KEY_PORT              GPIOB
#define KEY1_PIN              GPIO_Pin_12
#define KEY2_PIN              GPIO_Pin_13
#define KEY3_PIN              GPIO_Pin_14
#define KEY4_PIN              GPIO_Pin_15
#define KEY_COUNT             4

/* ---------- 蜂鸣器与 LED ---------- */
#define BEEP_PORT             GPIOB
#define BEEP_PIN              GPIO_Pin_8

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
