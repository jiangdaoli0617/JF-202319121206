# Libraries — 外部依赖（不提交，每台电脑本地放一份）

本目录放 CMSIS 与 ST 标准外设库（SPL V3.5.0）。**整个目录被 `.gitignore` 忽略**，
所以两台电脑上的这套文件是各自独立的，不会互相覆盖，也不需要合并。

下面是两条都验证过的放置路线，任选其一即可。

---

## 路线 A：从 Keil 器件包获取（用户自己的电脑，已验证）

通过 Keil pack 渠道拿文件，不依赖 ST 官网（ST 在部分网络环境下会被拦截）。

| 内容 | 来源路径 |
|------|---------|
| 器件头文件 | `<PACK>\Keil\STM32F1xx_DFP\2.2.0\Device\Include\` |
| 启动文件 / 系统文件 | `...\Device\Source\ARM\`、`...\Device\Source\` |
| SPL inc / src | `...\Device\StdPeriph_Driver\` |
| CMSIS 内核头文件 | `<PACK>\ARM\CMSIS\4.5.0\CMSIS\Include\`（新 Keil 也可是 `5.0.1`） |

**版本要求**：两台电脑统一用 **DFP 2.2.0**，配套 **Keil 5.24a**。
工程文件写的就是 2.2.0；不要再装/选 2.4.1。

> 用户电脑上 `<PACK>` = `G:\Keil_v5\ARM\PACK`，DFP 2.2.0 已装。
> 之前 5.21a + 新版 DFP 打开 STM32F103C8 工程会闪退，已改用 5.24a。

这条路线用的是 **CMSIS 4.x** 风格，`core_cm3.h` 会包含 `cmsis_armcc.h`，
所以 `cmsis_*.h` 必须一起复制：

```
Libraries/CMSIS/
├── core_cm3.h
├── core_cmFunc.h
├── core_cmInstr.h
├── core_cmSimd.h
├── cmsis_armcc.h
├── cmsis_armcc_V6.h
├── cmsis_gcc.h
├── stm32f10x.h
├── system_stm32f10x.c
├── system_stm32f10x.h
└── startup_stm32f10x_md.s
```

复制命令（cmd）：

```bat
set DFP=<PACK>\Keil\STM32F1xx_DFP\2.2.0
set CMSIS=<PACK>\ARM\CMSIS\4.5.0\CMSIS\Include
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

---

## 路线 B：从 ST 官方 SPL 压缩包获取（工位端，已验证）

来源：`江协科技\固件库\STM32F10x_StdPeriph_Lib_V3.5.0.zip`，解压后按下面复制。

这条路线用的是 **CMSIS 1.30** 风格：只有 `core_cm3.c` / `core_cm3.h`，
**没有** `cmsis_*.h`，`core_cm3.h` 自成一体，直接能编译。

```
Libraries/CMSIS/
├── core_cm3.c
├── core_cm3.h
├── stm32f10x.h
├── system_stm32f10x.c
├── system_stm32f10x.h
└── startup_stm32f10x_md.s
```

源文件在压缩包里对应的位置：

| 目标 | 压缩包内路径 |
|------|-------------|
| `CMSIS/core_cm3.c|.h` | `Libraries\CMSIS\CM3\CoreSupport\` |
| `CMSIS/stm32f10x.h` | `Libraries\CMSIS\CM3\DeviceSupport\ST\STM32F10x\` |
| `CMSIS/system_stm32f10x.c|.h` | 同上 |
| `CMSIS/startup_stm32f10x_md.s` | 同上 `\startup\arm\` |
| `STM32F10x_StdPeriph_Driver/inc|src` | `Libraries\STM32F10x_StdPeriph_Driver\` |

> **两条路线的差别只在 CMSIS 版本，工程都能编译。** 因为 `Libraries/` 不提交，
> 两台电脑各用各的路线不会冲突，不必强行统一。

---

## 为什么启动文件必须选 `md`

`md` = Medium Density，对应 64KB / 128KB Flash 的 STM32F103C8 / CB。
选错成 `hd`（大容量）或 `ld`（小容量）会导致中断向量表与内存布局不匹配，
典型现象是**上电立即 HardFault**。

---

## 需要加入 Keil 工程的 SPL 源文件

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
`core_cm3.c` 目前在工程里没有引用，也不需要——`core_cm3.h` 里的函数是内联的。
如果链接时真的报缺符号，再把它加进工程即可。

---

## `stm32f10x_conf.h`

本仓库已在 `Core/stm32f10x_conf.h` 提供精简版，只启用本项目用到的驱动，
无需再从 SPL 模板复制。若替换为模板原版，注意它默认开启 `USE_FULL_ASSERT`，
需要实现 `assert_failed()`，否则链接报错。

---

## 常见问题

| 现象 | 处理 |
|------|------|
| `cannot open source input file "cmsis_armcc.h"` | 用了路线 A 的 `core_cm3.h` 却没复制 `cmsis_*.h`；要么补齐，要么改用路线 B |
| `cannot open source input file "stm32f10x.h"` | 文件没复制，或 Include Path 缺 `Libraries/CMSIS` |
| 器件列表里找不到 STM32F103C8 | 器件包（DFP）没装。用 `江协科技\Keil5 MDK\支持包\Keil.STM32F1xx_DFP.2.2.0.pack` 双击安装 |
| 编译报 `core_cm3.h` 未定义符号 | 路线 A/B 混用了：`core_cm3.h` 与 `cmsis_*.h` 必须来自同一套 |
