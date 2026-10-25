# firmware — STM32 端工程

## 1. 前置条件

| 项 | 要求 |
|----|------|
| Keil MDK | 5.24 及以上（ARMCC v5 或 v6 均可） |
| 器件包 | `Keil.STM32F1xx_DFP` 2.x |
| 标准外设库 | STM32F10x_StdPeriph_Lib_V3.5.0 |
| 下载器 | ST-Link V2（SWD） |
| 目标芯片 | STM32F103C8T6 |

## 2. 重要：路径不能含中文

Keil 对非 ASCII 路径支持不稳定，常见症状是「找不到源文件」「生成的 hex 为空」
「编译器无输出」。当前仓库位于 `F:\毕设\JF-202319121206`，**含中文目录**。

正式编译前，把整个仓库复制到纯英文路径，例如：

```
F:\projects\JF-202319121206
```

Git 仓库本身可以留在原处，编译用的副本放在英文路径即可。

## 3. 补充依赖（必须手动完成一次）

本仓库不包含 ST 的标准外设库（体积大、受 ST 许可约束）。按 `Libraries/README.md`
把 SPL 复制到 `firmware/Libraries/`，否则编译会报：

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

本机未安装 Keil，无法预先验证 `.uvprojx`，因此工程文件未预置。
按 `Project/Keil配置说明.md` 手动创建，约 5 分钟；该文档列出了每个分组要添加的文件
和全部编译/调试选项。装好 Keil 后如需我生成并验证工程文件，告诉我器件包版本即可。

## 6. 编译前检查

- [ ] 工程已放在纯英文路径
- [ ] `Libraries/` 已按 `Libraries/README.md` 补齐
- [ ] Define 包含 `USE_STDPERIPH_DRIVER,STM32F10X_MD`
- [ ] Include Paths 包含 `Core`、`bsp`、`middle`、`protocol`、`app`、
      `Libraries/CMSIS`、`Libraries/STM32F10x_StdPeriph_Driver/inc`
- [ ] 启动文件为 `startup_stm32f10x_md.s`（中容量）
- [ ] 外部晶振 8 MHz，系统时钟 72 MHz

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
