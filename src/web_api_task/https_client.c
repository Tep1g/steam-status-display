#include "https_client.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

static char ssid[];
static char password[];

static bool https_client_inited = false;

void https_client_init() {
    if (https_client_inited) {
        return;
    }

    if(cyw43_arch_init_with_country(CYW43_COUNTRY_USA));

    cyw43_arch_enable_sta_mode();

    while(cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, 4000));

    https_client_inited = true;
}