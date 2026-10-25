#ifndef APP_UI_H
#define APP_UI_H

void app_ui_init(void);
void app_ui_show_idle(void);
void app_ui_show_prompt(const char *line1, const char *line2);
void app_ui_refresh(void);

#endif /* APP_UI_H */
