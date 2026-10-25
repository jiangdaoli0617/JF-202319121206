# firmware — STM32 端工程

## 1. 前置条件

| 项 | 本机实际情况 |
|----|------|
| Keil MDK | **5.21a**，ARM Compiler 5.06 update 3（`F:\UV4\UV4.exe`） |
| 器件包 | **Keil.STM32F1xx_DFP 2.3.0**（装在 `F:\ARM\PACK`，兼容 5.21a） |
| CMSIS | ARM.CMSIS 4.5.0（随 Keil 安装） |
| 标准外设库 | SPL V3.5.0，取自器件包的 `Device/StdPeriph_Driver` |
| 下载器 | ST-Link V2（SWD） |
| 目标芯片 | STM32F103C8T6（中容量，64 KB Flash / 20 KB RAM） |

> 用户口中的“STM32F108”不是真实型号；器件选择必须是 **STM32F103C8**。

## 2. 路径说明

当前仓库位于 `F:\毕设\JF-202319121206`，路径含中文。2026-10-25 已在本机用
Keil 5.21a 直接编译通过。若换机后出现「找不到源文件」「hex 为空」等异常，
再把仓库复制到纯英文路径（如 `F:\projects\JF-202319121206`）。

## 3. 补充依赖

本仓库不提交 SPL/CMSIS（已被 `.gitignore` 忽略）。本机已从
`Keil.STM32F1xx_DFP 2.3.0` 提取并放入 `firmware/Libraries/`。
换机时按 `Libraries/README.md` 重新放置，否则编译会报：

```
cannot open source input file "stm32f10x.h"
```

## 4. 目录结构

```
firmware/
├── Project/            Keil 工程与配置说明
├── Core/               启动、时钟、中断、延时、板级引脚定义
├── Libraries/          SPL 与 CMSIS（需手动复制，见 README）
├── bsp/                硬件驱动层：直接操作外设
├── middle/             抽象接口层：业务层只依赖这一层
├── protocol/           协议编解码，纯软件、无硬件依赖
├── app/                业务逻辑
└── main.c              初始化与主循环
```

分层规则见 `docs/04-软件模块与接口设计.md`：`app → middle → bsp`，
业务层不得包含任何 `bsp_*.h`。

## 5. Keil 工程

工程文件已预置：`firmware/Project/LabManager.uvprojx`，双击即可打开。

2026-10-25 用 Keil MDK 5.21a + ARMCC 5.06u3 实测：

```
0 Error(s), 2 Warning(s)
Program Size: Code=11894 RO-data=462 RW-data=124 ZI-data=3500
```

两条 warning 都来自尚未实现的桩函数（`drv_net.c` 的 `s_rx_cb`、`app_log.c` 的
`s_tail`），实现对应模块后自然消失。代码量 11.9 KB，**远低于 MDK-Lite 的 32 KB 上限**。

从零重建或换机的步骤见 `Project/Keil配置说明.md`。

## 6. 编译前检查

- [x] 工程文件 `Project/LabManager.uvprojx` 已生成
- [x] `Libraries/` 已按 `Libraries/README.md` 补齐（本机，git 不提交）
- [x] Define 包含 `USE_STDPERIPH_DRIVER,STM32F10X_MD`
- [x] Include Paths 包含 `Core`、`bsp`、`middle`、`protocol`、`app`、
      `Libraries/CMSIS`、`Libraries/STM32F10x_StdPeriph_Driver/inc`
- [x] 启动文件为 `startup_stm32f10x_md.s`（中容量）
- [x] 外部晶振 8 MHz，`system_stm32f10x.c` 配置为 72 MHz
- [ ] 实物烧录与分阶段验证（见第 7 节）

## 7. 分阶段验证顺序

不要一次接全部外设。按以下顺序，每一步单独验证通过后再进入下一步：

| 阶段 | 内容 | 通过标准 |
|------|------|---------|
| 1 | GPIO | 两个 LED 交替闪烁，蜂鸣器可控 |
| 2 | USART1 | 串口助手收到 `[BOOT] LabManager` |
| 3 | I2C 扫描 | 收到 `0x50 / 0x68 / 0x3C` 三个地址 |
| 4 | OLED | 能显示四行文字 |
| 5 | RC522 | 串口打印卡片 UID |
| 6 | DS3231 / AT24C02 | 时间可读写；EEPROM 写入后读回一致 |
| 7 | ESP8266 | AT 回显 OK，能连上 TCP 服务器 |

当前骨架中：阶段 1、2、3 的底层驱动已实现，可直接调用验证；
阶段 4~7 的设备驱动为桩函数，返回失败，待逐个实现。
