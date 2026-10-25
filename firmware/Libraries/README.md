# Libraries — 外部依赖

本目录需要手动放入 ST 的 CMSIS 与标准外设库（SPL）。仓库不提交这些文件。

## 1. 获取

下载 **STM32F10x_StdPeriph_Lib_V3.5.0**（ST 官网或常见的国内镜像），解压后得到：

```
STM32F10x_StdPeriph_Lib_V3.5.0/
├── Libraries/
│   ├── CMSIS/
│   │   └── CM3/
│   │       ├── CoreSupport/                     core_cm3.c / core_cm3.h
│   │       └── DeviceSupport/ST/STM32F10x/      stm32f10x.h
│   │           ├── system_stm32f10x.c / .h
│   │           └── startup/arm/                 startup_stm32f10x_md.s
│   └── STM32F10x_StdPeriph_Driver/
│       ├── inc/
│       └── src/
└── Project/STM32F10x_StdPeriph_Template/        stm32f10x_conf.h（可选）
```

## 2. 复制到本目录

完成后目录应为：

```
Libraries/
├── CMSIS/
│   ├── core_cm3.c
│   ├── core_cm3.h
│   ├── stm32f10x.h
│   ├── system_stm32f10x.c
│   ├── system_stm32f10x.h
│   └── startup_stm32f10x_md.s
└── STM32F10x_StdPeriph_Driver/
    ├── inc/        （全部 .h）
    └── src/        （全部 .c，工程中只添加实际用到的）
```

Windows 下可参考以下命令（把 `%SPL%` 换成你的解压路径）：

```bat
set SPL=D:\STM32F10x_StdPeriph_Lib_V3.5.0

copy "%SPL%\Libraries\CMSIS\CM3\CoreSupport\core_cm3.*"                    Libraries\CMSIS\
copy "%SPL%\Libraries\CMSIS\CM3\DeviceSupport\ST\STM32F10x\stm32f10x.h"    Libraries\CMSIS\
copy "%SPL%\Libraries\CMSIS\CM3\DeviceSupport\ST\STM32F10x\system_stm32f10x.*" Libraries\CMSIS\
copy "%SPL%\Libraries\CMSIS\CM3\DeviceSupport\ST\STM32F10x\startup\arm\startup_stm32f10x_md.s" Libraries\CMSIS\

xcopy /E /I "%SPL%\Libraries\STM32F10x_StdPeriph_Driver\inc" Libraries\STM32F10x_StdPeriph_Driver\inc
xcopy /E /I "%SPL%\Libraries\STM32F10x_StdPeriph_Driver\src" Libraries\STM32F10x_StdPeriph_Driver\src
```

## 3. 为什么启动文件选 `md`

`md` = Medium Density，对应 64KB / 128KB Flash 的 STM32F103C8/CB。
选错为 `hd`（大容量）或 `ld`（小容量）会导致中断向量表与内存布局不匹配，表现为
上电即 HardFault。

## 4. 需要加入 Keil 工程的 SPL 源文件

只添加实际用到的，避免编译时间与体积无谓增加：

```
misc.c
stm32f10x_gpio.c
stm32f10x_rcc.c
stm32f10x_spi.c
stm32f10x_i2c.c
stm32f10x_usart.c
```

`system_stm32f10x.c` 与 `startup_stm32f10x_md.s` 归入 Core 分组。

## 5. `stm32f10x_conf.h`

本仓库已在 `Core/stm32f10x_conf.h` 提供精简版，只启用本项目用到的驱动，
无需再从 SPL 模板复制。若你替换为模板原版，注意它默认开启 `USE_FULL_ASSERT`，
需要实现 `assert_failed()`，否则链接报错。
