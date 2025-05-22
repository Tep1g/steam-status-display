#include "https_client.h"
#include "http_client_util.h"
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "pico/cyw43_arch.h"
#include "tiny-json.h"

#define STEAM_DISPLAY_NAME_MAX_LEN 32
#define JSON_RESPONSE_MAX_SIZE 3000U
#define AVATAR_ICON_MAX_SIZE 8000U
#define GAME_ICON_MAX_SIZE 4000U
#define JSON_MAX_FIELDS 63U
#define MAX_URL_REQUEST_LEN 255U

static char ssid[];
static char password[];

#define STEAM_USER_ID ""
#define STEAM_WEB_API_KEY ""

#define API_HOSTNAME "api.steampowered.com"
#define AVATARS_HOSTNAME "avatars.steamstatic.com"
#define MEDIA_HOSTNAME "media.steampowered.com"
#define PLAYER_SUMMARIES_URL_REQUEST "/ISteamUser/GetPlayerSummaries/v0002/?key=" STEAM_WEB_API_KEY "&steamids=" STEAM_USER_ID
#define OWNED_GAMES_URL_REQUEST_PORTION "/IPlayerService/GetOwnedGames/v1/?key=" STEAM_WEB_API_KEY "&steamids=" STEAM_USER_ID "&include_appinfo=true&appids_filter[0]="
#define GAME_ICON_URL_REQUEST_PORTION "/steamcommunity/public/images/apps/"

static char url_request_buffer[MAX_URL_REQUEST_LEN*sizeof(char)];

static bool https_client_inited = false;

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
    .display_name = display_name_buf,
    .display_name_changed = false,
    .avatar_icon_jpg = avatar_icon_buf,
    .avatar_icon_changed = false,
    .game_icon_jpg = game_icon_buf,
    .game_icon_state = GAME_ICON_NO_CHANGE
};

struct steam_user_data_t *get_steam_user_data_ptr() {
    return &steam_user_data;
}

static void https_client_init() {
    if (https_client_inited) {
        return;
    }

    if(cyw43_arch_init_with_country(CYW43_COUNTRY_USA));

    cyw43_arch_enable_sta_mode();

    while(cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, 4000));

    https_client_inited = true;
}

static void make_http_request(void *cb_arg, altcp_recv_fn recv_fn, char *host, char *url_request) {
    HTTP_REQUEST_T request;
    request.callback_arg = cb_arg;
    request.hostname = host;
    request.url = url_request;
    request.headers_fn = http_client_header_callback;
    request.recv_fn = recv_fn;
    http_client_request_sync(cyw43_arch_async_context(), &request);
}

static void update_display_name(const json_t *player_property) {
    const json_t *display_name_field = json_getProperty(player_property, "personaname");
    const char *display_name = json_getValue(display_name_field);
    
    // the user's display name changed
    if (strcmp(display_name_buf, display_name) != 0) {
        strcpy(display_name_buf, display_name);
        steam_user_data.display_name_changed = true;
    }
}

static void update_avatar_icon(const json_t *player_property) {
    const json_t *avatar_hash_field = json_getProperty(player_property, "avatarhash");
    const char *avatar_hash = json_getValue(avatar_hash_field);

    // avatar icon changed
    if (strcmp(avatar_hash, resp_avatar_icon.hash) != 0) {
        steam_user_data.avatar_icon_changed = true;

        // get the new avatar icon
        strcpy(url_request_buffer, "/");
        strcat(url_request_buffer, avatar_hash);
        strcat(url_request_buffer, "_medium.jpg");
        resp_avatar_icon.size = 0;
        make_http_request((void *)&resp_avatar_icon, http_client_recv_jpg_callback, AVATARS_HOSTNAME, url_request_buffer);

        //update the cached avatar hash
        strcpy(resp_avatar_icon.hash, avatar_hash);

    }
}

static void update_game_icon(const json_t *player_property, json_t *json_mem) {
    const json_t *game_id_field = json_getProperty(player_property, "gameid");

    // the user is in-game
    if (game_id_field != NULL) {
        const char *game_id = json_getValue(game_id_field);

        strcpy(url_request_buffer, OWNED_GAMES_URL_REQUEST_PORTION);
        strcat(url_request_buffer, game_id);
        resp_json.len = 0;
        make_http_request((void *)&resp_json, http_client_recv_json_callback, API_HOSTNAME, url_request_buffer);
        resp_json.buf[resp_json.len] = '\0';

        const json_t *owned_games_response = json_create(resp_json.buf, json_mem, JSON_MAX_FIELDS);
        const json_t *games_list = json_getProperty(owned_games_response, "games");

        const json_t *game = json_getChild(games_list);
        const json_t *game_icon_hash_field = json_getProperty(game, "img_icon_url");

        const char *game_icon_hash = json_getValue(game_icon_hash_field);

        // game icon changed
        if (strcmp(game_icon_hash, resp_game_icon.hash) != 0) {
            steam_user_data.game_icon_state = GAME_ICON_SWITCHED;

            // get the new game icon
            strcpy(url_request_buffer, GAME_ICON_URL_REQUEST_PORTION);
            strcat(url_request_buffer, "/");
            strcat(url_request_buffer, game_icon_hash);
            strcat(url_request_buffer, ".jpg");
            resp_game_icon.size = 0;
            make_http_request((void *)&resp_game_icon, http_client_recv_jpg_callback, MEDIA_HOSTNAME, url_request_buffer);

            // update the cached game icon hash
            strcpy(resp_game_icon.hash, game_icon_hash);
        }
        
        // game icon is the same
        else {
            steam_user_data.game_icon_state = GAME_ICON_NO_CHANGE;
        }
    }
    // the user isn't in-game and was before this request
    else if (resp_game_icon.hash[0] != '\0') {
        steam_user_data.game_icon_state = GAME_ICON_CLEARED;
        resp_game_icon.hash[0] = '\0';
    }
    // the user isn't in-game and wasn't before this request
    else {
        steam_user_data.game_icon_state = GAME_ICON_NO_CHANGE;
    }
}

static void steam_user_data_update() {
    if (!https_client_inited) {
        return;
    }

    resp_json.len = 0;
    make_http_request((void *)&resp_json, http_client_recv_json_callback, API_HOSTNAME, PLAYER_SUMMARIES_URL_REQUEST);
    resp_json.buf[resp_json.len] = '\0';

    json_t json_mem[JSON_MAX_FIELDS];
    const json_t *summaries_response = json_create(resp_json.buf, json_mem, JSON_MAX_FIELDS);

    const json_t *players_list = json_getProperty(summaries_response, "players");
    const json_t *player = json_getChild(players_list);

    xSemaphoreTake(steam_user_data.mutex, 0);

    update_display_name(player);
    update_avatar_icon(player);
    update_game_icon(player, json_mem);

    steam_user_data.data_is_ready = true;
    xSemaphoreGive(steam_user_data.mutex);
}

void https_client_task(void *pvParameters) {
    https_client_init();

    while (1) {
        steam_user_data_update();
        sleep_ms(3200);
    }
}