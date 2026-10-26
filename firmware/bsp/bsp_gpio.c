#include "bsp_gpio.h"
#include "board_config.h"

static const uint16_t s_key_pins[KEY_COUNT] = {
    KEY1_PIN, KEY2_PIN, KEY3_PIN, KEY4_PIN, KEY5_PIN
};

static void set_output(GPIO_TypeDef *port, uint16_t pin,
                       uint8_t on, uint8_t active_high) {
    uint8_t level = ((on != 0U) == (active_high != 0U)) ? 1U : 0U;
    if (level) {
        GPIO_SetBits(port, pin);
    } else {
        GPIO_ResetBits(port, pin);
    }
}

void bsp_gpio_init(void) {
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB |
                           RCC_APB2Periph_GPIOC |
                           RCC_APB2Periph_AFIO, ENABLE);

    /* 按键：上拉输入，按下为低 */
    gpio.GPIO_Pin   = KEY1_PIN | KEY2_PIN | KEY3_PIN | KEY4_PIN | KEY5_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_IPU;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(KEY_PORT, &gpio);

    /* 蜂鸣器：推挽输出，默认关闭 */
    gpio.GPIO_Pin   = BEEP_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(BEEP_PORT, &gpio);
    GPIO_ResetBits(BEEP_PORT, BEEP_PIN);

    /* LED：推挽输出，默认熄灭 */
    gpio.GPIO_Pin = LED_GREEN_PIN;
    GPIO_Init(LED_GREEN_PORT, &gpio);
    set_output(LED_GREEN_PORT, LED_GREEN_PIN, 0U, LED_GREEN_ACTIVE_HIGH);

    gpio.GPIO_Pin = LED_RED_PIN;
    GPIO_Init(LED_RED_PORT, &gpio);
    set_output(LED_RED_PORT, LED_RED_PIN, 0U, LED_RED_ACTIVE_HIGH);
}

uint8_t bsp_key_raw(uint8_t index) {
    if (index >= KEY_COUNT) {
        return 0U;
    }
    return (GPIO_ReadInputDataBit(KEY_PORT, s_key_pins[index]) == Bit_RESET)
           ? 1U : 0U;
}

void bsp_led_green(uint8_t on) {
    set_output(LED_GREEN_PORT, LED_GREEN_PIN, on, LED_GREEN_ACTIVE_HIGH);
}

void bsp_led_red(uint8_t on) {
    set_output(LED_RED_PORT, LED_RED_PIN, on, LED_RED_ACTIVE_HIGH);
}

void bsp_beep(uint8_t on) {
    if (on) {
        GPIO_SetBits(BEEP_PORT, BEEP_PIN);
    } else {
        GPIO_ResetBits(BEEP_PORT, BEEP_PIN);
    }
}
