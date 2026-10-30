# firmware — STM32 端工程

## 1. 前置条件

| 项 | 本机实际情况 |
|----|------|
| Keil MDK | **5.24a**，ARM Compiler 5.06 update 5（`G:\Keil_v5\UV4\UV4.exe`） |
| 器件包 | **Keil.STM32F1xx_DFP 2.2.0**（`G:\Keil_v5\ARM\PACK`） |
| CMSIS | ARM.CMSIS 4.5.0 / 5.0.1（随器件包或 Keil 安装） |
| 标准外设库 | SPL V3.5.0，取自器件包的 `Device/StdPeriph_Driver` |
| 下载器 | ST-Link V2（SWD） |
| 目标芯片 | STM32F103C8T6（中容量，64 KB Flash / 20 KB RAM） |

> 用户口中的“STM32F108”不是真实型号；器件选择必须是 **STM32F103C8**。
> 旧 `G:\UV4`（5.21a）打开 STM32F103C8 工程会闪退，已改用 `G:\Keil_v5`（5.24a）。

## 2. 路径说明

当前仓库位于 `F:\毕设\JF-202319121206`，路径含中文。Keil 5.24a 下已实测可编译、
可双击打开；旧 5.21a 在中文路径下会 3~7 秒闪退，不要再用。

## 3. 补充依赖

本仓库不提交 SPL/CMSIS（已被 `.gitignore` 忽略）。本机已从
`Keil.STM32F1xx_DFP`（2.2.0 / 2.3.0）提取并放入 `firmware/Libraries/`。
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

2026-10-28 用 Keil MDK 5.24a（`G:\Keil_v5`）+ ARMCC 5.06u5 实测：

```
0 Error(s), 3 Warning(s)
Program Size: Code=6196 RO-data=2048 RW-data=116 ZI-data=4524
```

三条 warning 分别是 `bsp_oled.c` 的 `oled_write_cmd` 未引用、`drv_net.c` 的
`s_rx_cb`、`app_log.c` 的 `s_tail`，实现对应模块后自然消失。代码量 6.2 KB，
**远低于 MDK-Lite 的 32 KB 上限**。

> 工程固定使用 **DFP 2.2.0 + STM32F103C8**，不要退回 Keil 5.21a；
> 也不要让 Keil 改写 `LabManager.uvprojx` 后直接提交，避免给另一台制造差异。

从零重建或换机的步骤见 `Project/Keil配置说明.md`。

## 6. 编译前检查

- [x] 工程文件 `Project/LabManager.uvprojx` 已生成
- [x] Keil 5.24a + DFP 2.2.0，器件 `STM32F103C8`（64K/20K）
- [x] `Libraries/` 已按 `Libraries/README.md` 补齐（本机，git 不提交）
- [x] Define 包含 `USE_STDPERIPH_DRIVER,STM32F10X_MD`
- [x] Include Paths 包含 `Core`、`bsp`、`middle`、`protocol`、`app`、
      `Libraries/CMSIS`、`Libraries/STM32F10x_StdPeriph_Driver/inc`
- [x] 启动文件为 `startup_stm32f10x_md.s`（中容量）
- [x] 调试器 ST-Link，烧录算法 `STM32F10x_128.FLM`（2.2.0 自带）
- [ ] 实物烧录与分阶段验证（见第 7 节）

## 7. 分阶段验证顺序

不要一次接全部外设。按以下顺序，每一步单独验证通过后再进入下一步：

| 阶段 | 内容 | 通过标准 |
|------|------|---------|
| 1 | GPIO | 两个 LED 交替闪烁，蜂鸣器可控 |
| 2 | USART2 | 串口助手收到 `[BOOT] LabManager` |
| 3 | I2C 扫描 | 收到 `0x50 / 0x68 / 0x3C` 三个地址 |
| 4 | OLED | 能显示四行文字 |
| 5 | RC522 | 串口打印卡片 UID |
| 6 | DS3231 / AT24C02 | 时间可读写；EEPROM 写入后读回一致 |
| 7 | ESP8266 | AT 回显 OK，能连上 TCP 服务器 |

2026-10-28 实物验证进度：

- 阶段 2（USART2 启动打印 `[BOOT] LabManager`）**已通过**
- 阶段 4（OLED 显示 `Lab Manager` / `Swipe ID card`）**已通过**
- 阶段 1 的蜂鸣器、阶段 2、阶段 4、模块 3 按键（K1~K5）已实物验证通过
- 绿色 LED、阶段 3 的 I2C 地址扫描、阶段 5~7 尚未验证
- 模块 3 按键 + 蜂鸣器已通过：按 K1~K5（PB11~PB15）显示键名并短鸣一声
- 下一步：**模块 4 AT24C02**（PB6/PB7，地址 0x50）

当前骨架中：阶段 1~5 的底层驱动已实现（OLED、RC522 已写），阶段 6/7 与业务整合待做。
