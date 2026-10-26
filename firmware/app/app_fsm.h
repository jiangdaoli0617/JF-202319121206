#ifndef APP_FSM_H
#define APP_FSM_H

#include <stdint.h>

typedef enum {
    ST_INIT = 0,
    ST_IDLE,
    ST_AUTH_OK,
    ST_EXECUTE,
    ST_ADMIN_MENU
} app_state_t;

void        app_fsm_init(void);
void        app_fsm_run(void);      /* 主循环中反复调用 */
app_state_t app_fsm_state(void);

#endif /* APP_FSM_H */
