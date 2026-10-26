#include "delay.h"
#include "stm32f10x.h"

static uint32_t s_cycles_per_us = 72U;

void delay_init(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    s_cycles_per_us = SystemCoreClock / 1000000U;
}

void delay_us(uint32_t us) {
    uint32_t start  = DWT->CYCCNT;
    uint32_t cycles = us * s_cycles_per_us;

    while ((DWT->CYCCNT - start) < cycles) {
        __NOP();
    }
}

void delay_ms(uint32_t ms) {
    while (ms-- > 0U) {
        delay_us(1000U);
    }
}
