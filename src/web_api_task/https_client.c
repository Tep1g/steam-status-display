#include "https_client.h"
#include "http_client_util.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include <stdbool.h>

#define PLAYER_SUMMARIES_MAX_SIZE 3000
#define PLAYER_ICON_MAX_SIZE 8000
#define PLAYER_GAME_ICON_MAX_SIZE 4000 

static char ssid[];
static char password[];

static bool https_client_inited = false;

static struct steam_user_data_t steam_user_data = {
    .data_is_ready = false
};

struct steam_user_data_t *get_steam_user_data_ptr() {
    return &steam_user_data;
}

static char json_buf[PLAYER_SUMMARIES_MAX_SIZE*sizeof(char)];
static struct steam_response_json resp_json = {
    .buf = json_buf,
    .len = 0
};

static uint8_t avatar_icon_buf[PLAYER_ICON_MAX_SIZE*sizeof(uint8_t)];
static struct steam_response_jpg resp_avatar_icon = {
    .buf = avatar_icon_buf,
    .size = 0
};

static uint8_t game_icon_buf[PLAYER_GAME_ICON_MAX_SIZE*sizeof(uint8_t)];
static struct steam_response_jpg resp_game_icon = {
    .buf = game_icon_buf,
    .size = 0
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