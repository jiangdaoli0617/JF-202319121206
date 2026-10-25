# Keil 工程配置说明

目标：从零创建一个可编译、可下载、可调试的 STM32F103C8T6 工程。

**工程文件已预置并实测编译通过**：`firmware/Project/LabManager.uvprojx`。
本文保留为换机、从零重建或排查配置的参考。

---

## 0. 开始前

1. 本机实际使用的 Keil 在 `G:\`：`G:\UV4\UV4.exe`，ARM Compiler 5.06 update 3，
   许可证为 MDK-Lite（32 KB 代码上限）。`F:\UV4` 是同一份拷贝（MD5 相同），
   以 `G:` 为准。
2. `G:\ARM\PACK\Keil\STM32F1xx_DFP` 下已安装 **2.3.0 和 2.4.1**；本工程使用
   **2.4.1**（与江协模板一致）。
3. 按 `firmware/Libraries/README.md` 把 SPL/CMSIS 放入 `firmware/Libraries/`。
   本机已完成，该目录被 git 忽略。
4. 安装 ST-Link 驱动（Keil 安装目录 `G:\ARM\STLink\` 已自带）。

---

## 1. 新建工程

1. Keil → `Project` → `New uVision Project...`
2. 保存到 `firmware/Project/`，文件名 `LabManager`。
3. 器件选择：`STMicroelectronics` → `STM32F1 Series` → `STM32F103` → `STM32F103C8`。
4. 弹出的 `Manage Run-Time Environment` 直接点 **Cancel**。
   本项目使用自带的 SPL 与启动文件，不通过 RTE 引入，避免版本混杂。

---

## 2. 建立分组并添加文件

在 `Project` 窗口右键 `Target 1` → `Manage Project Items...`，
建立以下分组（Group），然后逐个 `Add Files`。

### Core

| 文件 | 位置 | 说明 |
|------|------|------|
| `main.c` | `firmware/main.c` | |
| `stm32f10x_it.c` | `firmware/Core/` | |
| `delay.c` | `firmware/Core/` | |
| `system_stm32f10x.c` | `firmware/Libraries/CMSIS/` | 从 SPL 复制 |
| `startup_stm32f10x_md.s` | `firmware/Libraries/CMSIS/` | 从 SPL 复制，**必须是 md** |

> 添加 `.s` 文件时，文件类型选 `All files (*.*)`，否则看不到。

### BSP

`firmware/bsp/` 下全部 `.c`：

```
bsp_gpio.c   bsp_spi.c    bsp_i2c.c     bsp_usart.c
bsp_rc522.c  bsp_ds3231.c bsp_at24c02.c bsp_oled.c  bsp_esp8266.c
```

### Middle

`firmware/middle/` 下全部 `.c`：

```
drv_rfid.c  drv_rtc.c  drv_storage.c  drv_display.c
drv_input.c drv_indicator.c  drv_net.c
```

### Protocol

`firmware/protocol/` 下全部 `.c`：

```
proto_crc.c  proto_tlv.c  proto_frame.c
```

### App

`firmware/app/` 下全部 `.c`：

```
app_config.c  app_auth.c  app_inventory.c  app_log.c  app_fsm.c  app_ui.c
```

### StdPeriph

`firmware/Libraries/STM32F10x_StdPeriph_Driver/src/` 下，只添加：

```
misc.c
stm32f10x_gpio.c
stm32f10x_rcc.c
stm32f10x_spi.c
stm32f10x_i2c.c
stm32f10x_usart.c
```

### CMSIS

本机使用 ARM.CMSIS 4.5.0，内核函数在 `core_cm3.h` 内联，**不需要 `core_cm3.c`**，
因此工程中没有 CMSIS 源文件分组。`core_cm3.h` 会包含 `cmsis_armcc.h`，
所以 `Libraries/CMSIS/` 下必须同时存在：

```
core_cm3.h  core_cmFunc.h  core_cmInstr.h  core_cmSimd.h
cmsis_armcc.h  cmsis_armcc_V6.h  cmsis_gcc.h
```

---

## 3. Options for Target

点击工具栏 `Options for Target`（魔术棒图标）。

### 3.1 Target 页

| 项 | 值 |
|----|-----|
| Xtal (MHz) | `8.0` |
| ARM Compiler | `Use default compiler version 5`（或 v6，见下） |
| Use MicroLIB | 勾选（可选，减小体积） |

若用 ARM Compiler 6，需在 `C/C++` 页把 `C99 Mode` 勾上，并把
`Misc Controls` 设为 `-Wno-license-management` 不是必需的，保持默认即可。

### 3.2 Output 页

| 项 | 值 |
|----|-----|
| Create HEX File | **勾选** |
| Select Folder for Objects | `.\Objects\` |
| Name of Executable | `LabManager` |

### 3.3 Listing 页

`Select Folder for Listings` → `.\Listings\`

### 3.4 C/C++ 页

**Define**（精确填写，逗号分隔，不要有空格）：

```
USE_STDPERIPH_DRIVER,STM32F10X_MD
```

**Include Paths**（逐个添加）：

```
..\Core
..\bsp
..\middle
..\protocol
..\app
..\Libraries\CMSIS
..\Libraries\STM32F10x_StdPeriph_Driver\inc
```

**Optimization**：调试阶段建议 `Level 0 (-O0)`，发布前改 `Level 2 (-O2)`。

**C99 Mode**：勾选。

### 3.5 Debug 页

1. 右上角选择 `ST-Link Debugger`。
2. 点 `Settings`：
   - `Debug` 选项卡：`Port` 选 `SW`，`Max Clock` 保持默认。
   - `Flash Download` 选项卡：勾选 `Reset and Run`；
     确认 Programming Algorithm 中有
     `STM32F10x Med-density Flash 64KB`，没有则点 `Add` 添加。
3. 若下载报 `No target connected`：检查 SWDIO/SWCLK/GND/3.3V 四根线，
   并确认板子已上电。

### 3.6 Utilities 页

勾选 `Use Target Driver for Flash Programming`，下拉选 `ST-Link Debugger`，
并勾选 `Update Target before Debugging`。

---

## 4. 编译与验证

按 `F7` 编译。首次应无错误（0 Error）。若报错，对照第 5 节。

下载前先做最小验证：

1. 打开 `main.c`，确认 `main()` 中调用链完整。
2. 编译通过后按 `F8` 下载。
3. 串口助手（115200-8-N-1）接 USART1（PA9/PA10），应看到启动打印。

---

## 5. 常见错误对照

| 报错 | 原因 | 处理 |
|------|------|------|
| `*** Target ... uses RTOS support that is part of a Legacy Pack` | 工程模板继承了 `ArmAdsMisc/mOS=1`（RTX 标志） | 把 `.uvprojx` 中 `mOS` 改为 `0`；本项目不使用 RTOS |
| 打开工程时 Keil 直接闪退（异常码 `0xc0000005`） | `.uvprojx` 用了新版 pack 示例的 XML 结构，5.21 的 GUI 解析崩溃 | 用江协模板的 5.21 兼容结构重建；本项目已处理，不要再从 Blinky 示例改工程 |
| `cannot open source input file "stm32f10x.h"` | SPL 未复制或 Include Path 缺 `Libraries/CMSIS` | 按 `Libraries/README.md` 补齐 |
| `cannot open source input file "core_cm3.h"` | 同上 | 同上 |
| `cannot open source input file "cmsis_armcc.h"` | CMSIS 头文件只复制了 `core_cm3.h` | 把 `cmsis_armcc.h` 等一并复制到 `Libraries/CMSIS/` |
| `Undefined symbol SystemInit` | 没把 `system_stm32f10x.c` 加入工程 | 加入 Core 分组 |
| `Undefined symbol __main` / 启动失败 | 启动文件未加入或选错容量 | 加入 `startup_stm32f10x_md.s` |
| `L6915E: Library reports error: __use_no_semihosting` | 用了 printf 但未重定向 | 见下方「printf 重定向」 |
| 下载后无反应 | 启动文件容量选错 / 时钟未起振 | 确认 `md`；检查 8 MHz 晶振 |
| `No target connected` | SWD 接线或供电 | 检查四线，给板子独立供电 |
| 编译输出目录为空 | 路径异常 | 把仓库复制到纯英文路径再试 |

### printf 重定向（可选）

骨架暂未使用 `printf`，调试打印请用 `bsp_usart_send_str(USART_PORT_DEBUG, "...")`。
若确实要用 `printf`，在 `bsp_usart.c` 中加入：

```c
#include <stdio.h>

int fputc(int ch, FILE *f) {
    (void)f;
    bsp_usart_send(USART_PORT_DEBUG, (const uint8_t *)&ch, 1);
    return ch;
}
```

并在 Options → Target 勾选 `Use MicroLIB`，否则需额外实现半主机退出函数。

---

## 6. 工程目录约定

`Objects/`、`Listings/`、`*.uvguix.*`、`*.hex`、`*.axf` 均已加入 `.gitignore`，
不需要提交。工程文件 `LabManager.uvprojx` 与 `LabManager.uvoptx` 应当提交，
以便换机后直接打开。

---

## 7. 验证清单

- [x] 0 Error（本机实测：0 Error，2 条桩函数 warning）
- [x] Code=6104 B，RO=452 B，RW=112 B，ZI=3496 B
- [x] 生成的 `LabManager.hex` 不为空
- [ ] ST-Link 能识别芯片
- [ ] 下载后复位，串口有启动打印
- [ ] 两个 LED 按预期闪烁
