#include "https_client.h"
#include "http_client_util.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include <stdbool.h>

#define STEAM_DISPLAY_NAME_MAX_LEN 32
#define JSON_RESPONSE_MAX_SIZE 3000U
#define AVATAR_ICON_MAX_SIZE 8000U
#define GAME_ICON_MAX_SIZE 4000U

static char ssid[];
static char password[];

static bool https_client_inited = false;

struct steam_user_data_t *get_steam_user_data_ptr() {
    return &steam_user_data;
}

static char display_name_buf[STEAM_DISPLAY_NAME_MAX_LEN+1];

static char json_buf[JSON_RESPONSE_MAX_SIZE*sizeof(char)];
static struct steam_response_json resp_json = {
    .buf = json_buf,
    .len = 0
};

static uint8_t avatar_icon_buf[AVATAR_ICON_MAX_SIZE*sizeof(uint8_t)];
static struct steam_response_jpg resp_avatar_icon = {
    .buf = avatar_icon_buf,
    .hash = {0},
    .size = 0
};

static uint8_t game_icon_buf[GAME_ICON_MAX_SIZE*sizeof(uint8_t)];
static struct steam_response_jpg resp_game_icon = {
    .buf = game_icon_buf,
    .hash = {0},
    .size = 0
};

static struct steam_user_data_t steam_user_data = {
    .data_is_ready = false,
    .display_name_changed = false,
    .avatar_icon_changed = false,
    .game_icon_state = GAME_ICON_NO_CHANGE
};

void https_client_init() {
    if (https_client_inited) {
        return;
    }

    if(cyw43_arch_init_with_country(CYW43_COUNTRY_USA));

    cyw43_arch_enable_sta_mode();

    while(cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, 4000));

    https_client_inited = true;
}