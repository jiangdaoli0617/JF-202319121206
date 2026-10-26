#ifndef __STM32F10x_CONF_H
#define __STM32F10x_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

/* 只启用本项目实际用到的外设驱动，缩短编译时间、减小体积。
 * 新增外设时在这里补一行 #include。 */
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_spi.h"
#include "stm32f10x_i2c.h"
#include "stm32f10x_usart.h"
#include "misc.h"

/* 不启用 USE_FULL_ASSERT：若启用，必须实现 assert_failed() */
#ifdef USE_FULL_ASSERT
  #define assert_param(expr) ((expr) ? (void)0 : assert_failed((uint8_t *)__FILE__, __LINE__))
  void assert_failed(uint8_t *file, uint32_t line);
#else
  #define assert_param(expr) ((void)0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __STM32F10x_CONF_H */
