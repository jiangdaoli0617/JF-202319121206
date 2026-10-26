# CLAUDE.md — 项目约定与协作说明

> 这个文件被两台开发电脑上的 Claude Code 自动读取，用来保持两边认知一致。
> 修改约定时**必须改这个文件并提交**，只改本地记忆另一台电脑看不到。

## 项目

本科毕业设计：**基于 STM32F103C8T6 标准库（StdPeriph）的实验室物品出入管理系统**。
现场端用 RFID 识别物品、按键借还、OLED 显示、声光提示、AT24C02 断电保存、DS3231 打时间戳、ESP-01S 经 WiFi 上传给 Java 上位机。

**禁止使用 HAL 库**，全程标准库。

## 引脚与接口方案（唯一权威：`开题报告.md` 的接口表）

```
RC522 (SPI1)    CS=PA4  SCK=PA5  MISO=PA6  MOSI=PA7   RST=PB0
软件 I2C 共用    SCL=PB6  SDA=PB7    OLED 0x3C / AT24C02 0x50 / DS3231 0x68（7 位地址）
按键 ×5         登记=PB11  借出=PB12  归还=PB13  上一条=PB14  下一条=PB15（上拉，按下接地）
蜂鸣器          PB8（S8050 驱动，高电平响）
ESP-01S         USART1  PA9(TX)/PA10(RX)
调试串口        USART2  PA2(TX)/PA3(RX) → USB-TTL，printf
板载 LED        PC13（低电平点亮）；PB9 绿色 LED 为预留，未在开题报告中
```

**任何与上表冲突的方案一律以上表为准。** 所有引脚宏集中定义在 `firmware/Core/board_config.h`，驱动 `.c` 里禁止硬编码引脚号。

## 目录结构

```
开题报告.md          ← 接口方案的权威来源
firmware/            ← 唯一的固件工程（Keil 工程在 firmware/Project/）
  Core/                启动、时钟、中断、延时、board_config.h
  bsp/                 硬件驱动：GPIO / I2C / SPI / USART / OLED / AT24C02 / DS3231 / RC522 / ESP
  middle/              设备抽象层 drv_*：显示、按键、声光、RFID、RTC、存储、网络
  protocol/            报文打包与 CRC
  app/                 业务层：状态机、权限、台账、日志、界面
  Libraries/           CMSIS + 标准库，被 .gitignore 忽略，每台机器本地放置
docs/                ← 设计文档（需求、架构、硬件、模块、规范、难点、EEPROM）
江协科技/            ← 本地参考资料，3.2 GB，已被 .gitignore 忽略
```

分层依赖方向固定：`app → middle → bsp`。业务层只 include `drv_*.h`，不碰 `bsp_*.h`。

## 代码规范（用户强制要求）

1. 分模块开发，一次只写一个模块，上机验证通过后再做下一个。
2. 详细中文注释；函数功能单一；命名通俗，方便答辩讲解。
3. `.c` 与 `.h` 分开，严格区分驱动层与业务逻辑层。
4. 每份交付附 4 项：① 原理说明 ② Keil 编译注意事项 ③ 硬件接线 ④ 上机测试步骤。
5. 发现用户给的参数有逻辑矛盾或硬件冲突时，先指出确认，不要默认用户全对。

开发顺序：GPIO → OLED → 按键+蜂鸣器 → AT24C02 → DS3231 → RC522 → ESP-01S。

## 两台电脑协作流程（重要）

本项目由两台电脑双向开发：用户自己的电脑 + 工位端。

**每次开始开发前，先 `git fetch origin` + `git status -sb` 确认本地与远程是否一致：**
落后就先 `git pull`，分叉就先合并，一致才开始写代码。

提交信息写清楚「改了什么、为什么」，因为另一台电脑上的人只能靠它理解这边的思路。

## 当前进度（2026-10-26）

- 已完成：工程骨架 + Keil 工程；模块 1 延时与 GPIO；模块 2 OLED 驱动（`bsp_oled.c` / `bsp_oled_font.h`）
- 汉字 16×16 字库目前是占位模板，需用 PCtoLCD2002 取模后填入
- 下一步：模块 3 按键 + 蜂鸣器
- `firmware/Libraries/`（CMSIS + 标准库）需在每台机器本地放置，来源见 `firmware/Libraries/README.md`

## 参考资料注意事项

`江协科技/` 是通用入门教程，**不能直接照抄**：

- 他的引脚与本站方案冲突（OLED 用 PB8/PB9、软件 I2C 用 PB10/PB11；而 PB8 是本项目的蜂鸣器、PB11 是「登记」按键）。
- 他的套件里**没有** RC522、DS3231、AT24C02、ESP-01S 这四个本项目的核心模块，相关驱动需按芯片数据手册自行实现。
- 可复用的只有：8×16 ASCII 字模、SSD1306 初始化序列思路、软件 I2C 时序写法、串口收发参考。
