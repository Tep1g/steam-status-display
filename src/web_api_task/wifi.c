#include "wifi.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

static char ssid[];
static char password[];

enum wifi_return_status wifi_init_and_connect() {
    if(cyw43_arch_init_with_country(CYW43_COUNTRY_USA)) {
        return WIFI_STATUS_ERROR;
    }

    cyw43_arch_enable_sta_mode();

    if(cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        return WIFI_STATUS_ERROR;
    }

    return WIFI_STATUS_OK;
}