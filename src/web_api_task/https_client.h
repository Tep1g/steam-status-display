#ifndef HTTPS_CLIENT_H
#define HTTPS_CLIENT_H

#include <stdbool.h>
#include <semphr.h>

enum game_icon_state_t {
    GAME_ICON_SWITCHED,
    GAME_ICON_CLEARED,
    GAME_ICON_NO_CHANGE,
};

struct steam_user_data_t {
    bool data_is_ready;
    SemaphoreHandle_t mutex;
    char *display_name;
    bool display_name_changed;
    uint8_t *avatar_icon_jpg;
    bool avatar_icon_changed;
    uint8_t *game_icon_jpg;
    enum game_icon_state_t game_icon_state;
};

struct steam_user_data_t *get_steam_user_data_ptr(void);

void https_client_task(void *pvParameters);

#endif /* HTTPS_CLIENT_H */