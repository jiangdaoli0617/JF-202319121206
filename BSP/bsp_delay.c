#include "bsp_delay.h"

/**
  ******************************************************************************
  * @file    bsp_delay.c
  * @brief   延时函数驱动（基于 SysTick 系统滴答定时器）
  ******************************************************************************
  * 【原理说明】
  * SysTick 是 Cortex-M3 内核自带的一个 24 位递减计数器，不属于任何外设，
  * 因此不需要开外设时钟、不占用 GPIO，非常适合做系统延时。
  *
  * 本模块用了两种工作方式：
  *   1) 毫秒延时：让 SysTick 每 1ms 产生一次中断，在中断里给全局变量
  *      s_tick_ms 加 1。Delay_Ms 只需要比较这个变量的差值来等待。
  *   2) 微秒延时：临时把 SysTick 的重装载值改成"微秒数 × 72"，让它数完
  *      指定的节拍后自己停在 COUNTFLAG 标志位上，我们用死循环去等这个标志。
  *      用完后把 SysTick 还原成 1ms 中断的配置。
  *      之所以用"×72"，是因为系统主频 72MHz，一个微秒正好是 72 个时钟周期。
  ******************************************************************************
  */

/** 开机至今经过的毫秒数，在 SysTick 中断里自增。
 *  加 volatile 是防止编译器优化掉"看起来没人改"的变量。 */
static volatile uint32_t s_tick_ms = 0;


/* ============================ 内部函数（只给本文件用） ============================ */

/**
  * @brief  一次性微秒延时（不做分段，调用者需保证 us 不超过约 200000）
  * @param  us: 微秒数，必须大于 0
  * @note   函数只做一件事：精确地等 us 个微秒，然后恢复 SysTick 原状。
  */
static void delay_us_once(uint32_t us)
{
    uint32_t old_load;      /* 保存原来的重装载值，用完要还回去 */
    uint32_t old_ctrl;      /* 保存原来的控制寄存器值 */
    uint32_t ticks;         /* 需要数多少个时钟周期 */
    uint32_t ctrl_now;      /* 循环里读到的控制寄存器值 */

    old_load = SysTick->LOAD;
    old_ctrl = SysTick->CTRL;

    /* 1 微秒 = SystemCoreClock / 1000000 = 72 个时钟周期 */
    ticks = us * (SystemCoreClock / 1000000U);

    /* ---------- 第一步：停掉 SysTick，改成纯计数、不产生中断 ---------- */
    SysTick->CTRL = 0;                              /* 关闭计数与中断 */
    SysTick->LOAD = ticks - 1U;                     /* 数 ticks 个周期后归零 */
    SysTick->VAL  = 0;                              /* 清空当前计数值和 COUNTFLAG */
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |    /* 时钟源 = 主频 72MHz */
                    SysTick_CTRL_ENABLE_Msk;        /* 启动计数（不使能中断） */

    /* ---------- 第二步：死等，直到数到 0（COUNTFLAG 被硬件置 1） ---------- */
    do
    {
        ctrl_now = SysTick->CTRL;
    }
    while (((ctrl_now & SysTick_CTRL_ENABLE_Msk) != 0U) &&
           ((ctrl_now & SysTick_CTRL_COUNTFLAG_Msk) == 0U));

    /* ---------- 第三步：把 SysTick 还原成"1ms 中断"的配置 ---------- */
    SysTick->CTRL = 0;
    SysTick->LOAD = old_load;
    SysTick->VAL  = 0;
    SysTick->CTRL = old_ctrl;
}


/* ============================== 对外函数 ============================== */

/**
  * @brief  延时模块初始化
  */
void Delay_Init(void)
{
    /* SysTick_Config 的参数是"多少个时钟周期中断一次"。
     * SystemCoreClock/1000 = 72000，即 72000 个周期 = 1ms。
     * 返回值非 0 表示重装载值超出 24 位范围，配置失败。 */
    if (SysTick_Config(SystemCoreClock / 1000U) != 0U)
    {
        while (1)       /* 配置失败，停在这里方便调试时发现 */
        {
        }
    }

    /* 把 SysTick 中断优先级设为最低，避免它打断其它外设的中断 */
    NVIC_SetPriority(SysTick_IRQn, 0x0F);
}

/**
  * @brief  SysTick 中断服务函数
  * @note   名字固定不能改，由启动文件中的中断向量表调用。
  *         注意：如果 Keil 工程里的 stm32f10x_it.c 文件中已经有一个
  *         空的 SysTick_Handler，必须把那一个注释掉，否则会重复定义报错！
  */
void SysTick_Handler(void)
{
    s_tick_ms++;
}

/**
  * @brief  获取开机至今的毫秒数
  */
uint32_t Delay_GetTick(void)
{
    return s_tick_ms;
}

/**
  * @brief  微秒级延时
  */
void Delay_Us(uint32_t us)
{
    /* SysTick 的 LOAD 寄存器只有 24 位，一次最多数 0xFFFFFF 个周期。
     * 为确保安全，超过 200000us（200ms）就拆成几段分别延时。 */
    while (us > 0U)
    {
        uint32_t step = (us > 200000U) ? 200000U : us;

        delay_us_once(step);
        us -= step;
    }
}

/**
  * @brief  毫秒级延时
  */
void Delay_Ms(uint32_t ms)
{
    uint32_t start_tick = s_tick_ms;

    /* 用无符号减法比较，即使 s_tick_ms 溢出回绕，结果依然正确 */
    while ((s_tick_ms - start_tick) < ms)
    {
        /* 空循环，等待 SysTick 中断自动累加 s_tick_ms */
    }
}
