#include "stm32f10x.h"
#include "board_config.h"

/* 1 ms 系统节拍，供消抖、超时、非阻塞状态机使用。
 * 主循环中的逻辑一律基于此时间戳，不使用 delay_ms() 等待。 */
volatile uint32_t g_sys_tick_ms = 0U;

void SysTick_Handler(void) {
    g_sys_tick_ms++;
}

/* 串口接收暂用轮询，若后续改为中断接收，在此实现：
 * USART1_IRQHandler / USART2_IRQHandler */
