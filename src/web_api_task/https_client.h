#ifndef TLS_CLIENT_H
#define TLS_CLIENT_H

#include <stdbool.h>
#include <semphr.h>

struct steam_user_data_t {
    bool data_is_ready;
    SemaphoreHandle_t mutex;
    char *display_name;
    uint8_t *avatar_icon_jpg;
    uint8_t *game_icon_jpg;
};

struct steam_user_data_t *get_steam_user_data_ptr(void);

void https_client_init();

#endif /* HTTPS_CLIENT_H */