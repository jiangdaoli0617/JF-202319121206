#ifndef __BSP_DELAY_H
#define __BSP_DELAY_H

/**
  ******************************************************************************
  * @file    bsp_delay.h
  * @brief   延时函数驱动（基于 SysTick 系统滴答定时器）
  * @note    这是整个工程最基础的模块，其它模块都依赖它：
  *          - LED 闪烁需要 Delay_Ms
  *          - 软件模拟 I2C 需要 Delay_Us 产生时序
  *          - 按键消抖需要 Delay_Ms
  ******************************************************************************
  */

#include "stm32f10x.h"


/* ------------------------------ 对外函数 ------------------------------ */

/**
  * @brief  延时模块初始化
  * @note   把 SysTick 配置成每 1ms 产生一次中断，用于毫秒级延时；
  *         同时启动一个全局毫秒计数器（s_tick_ms），后续记录时间、
  *         按键消抖都可以用它。必须在 main 函数开头调用一次。
  */
void Delay_Init(void);

/**
  * @brief  微秒级延时（阻塞式）
  * @param  us: 要延时的微秒数
  * @note   软件模拟 I2C 的时序就靠它，精度约 ±1us。
  *         内部会自动分段，所以传很大的值也不会出错。
  */
void Delay_Us(uint32_t us);

/**
  * @brief  毫秒级延时（阻塞式）
  * @param  ms: 要延时的毫秒数
  */
void Delay_Ms(uint32_t ms);

/**
  * @brief  获取系统开机至今的毫秒数
  * @return 开机以来经过的毫秒数（约 49 天会溢出回绕，本项目够用）
  * @note   和 Delay_Ms 配合可以做非阻塞延时、超时判断。
  */
uint32_t Delay_GetTick(void);


#endif /* __BSP_DELAY_H */
