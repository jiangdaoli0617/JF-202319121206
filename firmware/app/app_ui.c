#include "app_ui.h"
#include "drv_display.h"

void app_ui_init(void) {
    app_ui_show_idle();
}

void app_ui_show_idle(void) {
    drv_display_clear();
    drv_display_line(0U, "Lab Manager");
    drv_display_line(2U, "Swipe ID card");
    drv_display_refresh();
}

void app_ui_show_prompt(const char *line1, const char *line2) {
    drv_display_clear();
    if (line1 != 0) {
        drv_display_line(1U, line1);
    }
    if (line2 != 0) {
        drv_display_line(3U, line2);
    }
    drv_display_refresh();
}

void app_ui_refresh(void) {
    drv_display_refresh();
}
