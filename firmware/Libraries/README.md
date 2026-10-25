# Libraries — 外部依赖

本目录放 CMSIS 与 ST 标准外设库（SPL V3.5.0）。**该目录被 `.gitignore` 忽略，不提交**。
本文记录本机已验证的放置方式，换机时照做即可。

## 1. 来源

本机通过 Keil 的 pack 渠道获取，不依赖 ST 官网（ST 在本机被网络策略拦截，HTTP 567）：

| 内容 | 来源路径 |
|------|---------|
| 器件头文件 | `F:\ARM\PACK\Keil\STM32F1xx_DFP\2.3.0\Device\Include\` |
| 启动文件 / 系统文件 | `...\Device\Source\ARM\`、`...\Device\Source\` |
| SPL inc / src | `...\Device\StdPeriph_Driver\` |
| CMSIS 内核头文件 | `F:\ARM\PACK\ARM\CMSIS\4.5.0\CMSIS\Include\` |

**版本选择**：必须用 DFP **2.3.0**（pack schema 1.4.0，SPL V3.5.0，兼容 Keil 5.21a）。
2.4.1 的 schema 是 1.7.2，Keil 5.21a 识别不了。

## 2. 放置后的目录

```
Libraries/
├── CMSIS/
│   ├── core_cm3.h
│   ├── core_cmFunc.h
│   ├── core_cmInstr.h
│   ├── core_cmSimd.h
│   ├── cmsis_armcc.h
│   ├── cmsis_armcc_V6.h
│   ├── cmsis_gcc.h
│   ├── stm32f10x.h
│   ├── system_stm32f10x.c
│   ├── system_stm32f10x.h
│   └── startup_stm32f10x_md.s
└── STM32F10x_StdPeriph_Driver/
    ├── inc/        （全部 .h）
    └── src/        （全部 .c，工程中只添加实际用到的）
```

> CMSIS 4.5.0 没有 `core_cm3.c`，内核函数在 `core_cm3.h` 中内联，**不需要**该文件。
> 但 `core_cm3.h` 会包含 `cmsis_armcc.h`，所以 `cmsis_*.h` 必须一起复制。

## 3. 复制命令（cmd，管理员权限不需要）

```bat
set DFP=F:\ARM\PACK\Keil\STM32F1xx_DFP\2.3.0
set CMSIS=F:\ARM\PACK\ARM\CMSIS\4.5.0\CMSIS\Include
set LIB=firmware\Libraries

mkdir "%LIB%\CMSIS"
mkdir "%LIB%\STM32F10x_StdPeriph_Driver\inc"
mkdir "%LIB%\STM32F10x_StdPeriph_Driver\src"

copy "%DFP%\Device\Include\stm32f10x.h"        "%LIB%\CMSIS\"
copy "%DFP%\Device\Include\system_stm32f10x.h" "%LIB%\CMSIS\"
copy "%DFP%\Device\Source\system_stm32f10x.c"  "%LIB%\CMSIS\"
copy "%DFP%\Device\Source\ARM\startup_stm32f10x_md.s" "%LIB%\CMSIS\"

copy "%CMSIS%\core_cm3.h"     "%LIB%\CMSIS\"
copy "%CMSIS%\core_cmFunc.h"  "%LIB%\CMSIS\"
copy "%CMSIS%\core_cmInstr.h" "%LIB%\CMSIS\"
copy "%CMSIS%\core_cmSimd.h"  "%LIB%\CMSIS\"
copy "%CMSIS%\cmsis_*.h"      "%LIB%\CMSIS\"

xcopy /E /I /Y "%DFP%\Device\StdPeriph_Driver\inc" "%LIB%\STM32F10x_StdPeriph_Driver\inc"
xcopy /E /I /Y "%DFP%\Device\StdPeriph_Driver\src" "%LIB%\STM32F10x_StdPeriph_Driver\src"
```

## 4. 为什么启动文件选 `md`

`md` = Medium Density，对应 64KB / 128KB Flash 的 STM32F103C8/CB。
选错为 `hd`（大容量）或 `ld`（小容量）会导致中断向量表与内存布局不匹配，表现为
上电即 HardFault。

## 5. 需要加入 Keil 工程的 SPL 源文件

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

## 6. `stm32f10x_conf.h`

本仓库已在 `Core/stm32f10x_conf.h` 提供精简版，只启用本项目用到的驱动，
无需再从 SPL 模板复制。若你替换为模板原版，注意它默认开启 `USE_FULL_ASSERT`，
需要实现 `assert_failed()`，否则链接报错。

## 7. 常见问题

| 现象 | 处理 |
|------|------|
| `cannot open source input file "cmsis_armcc.h"` | `cmsis_*.h` 没复制全，按第 3 节补齐 |
| `cannot open source input file "stm32f10x.h"` | `stm32f10x.h` 没复制，或 Include Path 缺 `Libraries/CMSIS` |
| 器件列表里找不到 STM32F103C8 | DFP 版本不对或没装；用 2.3.0，不要 2.4.1 |
