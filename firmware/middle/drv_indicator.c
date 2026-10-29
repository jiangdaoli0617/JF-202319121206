#include "drv_indicator.h"
#include "bsp_gpio.h"
#include "board_config.h"

typedef struct {
    uint8_t  green;
    uint8_t  red;
    uint8_t  beep;
    uint16_t duration_ms;   /* 0 = 结束标记 */
} ind_step_t;

typedef struct {
    const ind_step_t *steps;
    uint8_t loop;           /* 1 = 播完从头开始 */
} ind_pattern_t;

static const ind_step_t STEP_OFF = {0U, 0U, 0U, 0U};

static const ind_step_t P_LEND_OK[] = {
    {1U, 0U, 1U, 80U},
    {1U, 0U, 0U, 220U},
    {0U, 0U, 0U, 0U}
};

static const ind_step_t P_REJECT[] = {
    {0U, 1U, 1U, 100U},
    {0U, 0U, 0U, 100U},
    {0U, 1U, 1U, 100U},
    {0U, 0U, 0U, 100U},
    {0U, 0U, 0U, 0U}
};

static const ind_step_t P_UNKNOWN[] = {
    {0U, 1U, 1U, 800U},
    {0U, 0U, 0U, 0U}
};

static const ind_step_t P_ERROR[] = {
    {0U, 1U, 1U, 80U}, {0U, 0U, 0U, 80U},
    {0U, 1U, 1U, 80U}, {0U, 0U, 0U, 80U},
    {0U, 1U, 1U, 80U}, {0U, 0U, 0U, 80U},
    {0U, 1U, 1U, 80U}, {0U, 0U, 0U, 80U},
    {0U, 1U, 1U, 80U},
    {0U, 0U, 0U, 0U}
};

static const ind_step_t P_NET_DOWN[] = {
    {0U, 1U, 0U, 500U},
    {0U, 0U, 0U, 500U},
    {0U, 0U, 0U, 0U}
};

/* 按键确认：只短鸣 60 ms，不亮灯 */
static const ind_step_t P_KEY[] = {
    {0U, 0U, 1U, 60U},
    {0U, 0U, 0U, 0U}
};

static const ind_pattern_t s_patterns[] = {
    {P_LEND_OK,  0U},
    {P_REJECT,   0U},
    {P_UNKNOWN,  0U},
    {P_ERROR,    0U},
    {P_NET_DOWN, 1U},
    {P_KEY,      0U}
};

static const ind_pattern_t *s_pattern = 0;
static uint8_t  s_step = 0U;
static uint32_t s_step_start = 0U;

static void apply_step(const ind_step_t *s) {
    bsp_led_green(s->green);
    bsp_led_red(s->red);
    bsp_beep(s->beep);
}

void drv_indicator_init(void) {
    s_pattern = 0;
    apply_step(&STEP_OFF);
}

void drv_indicator_show(ind_event_t ev) {
    if (ev >= IND_IDLE) {
        s_pattern = 0;
        apply_step(&STEP_OFF);
        return;
    }

    s_pattern = &s_patterns[ev];
    s_step = 0U;
    s_step_start = g_sys_tick_ms;
    apply_step(&s_pattern->steps[0]);
}

void drv_indicator_poll(void) {
    const ind_step_t *cur;

    if (s_pattern == 0) {
        return;
    }

    cur = &s_pattern->steps[s_step];
    if ((uint32_t)(g_sys_tick_ms - s_step_start) < cur->duration_ms) {
        return;
    }

    s_step++;
    if (s_pattern->steps[s_step].duration_ms == 0U) {
        if (s_pattern->loop) {
            s_step = 0U;
        } else {
            s_pattern = 0;
            apply_step(&STEP_OFF);
            return;
        }
    }

    s_step_start = g_sys_tick_ms;
    apply_step(&s_pattern->steps[s_step]);
}
