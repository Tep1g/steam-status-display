#ifndef WIFI_H
#define WIFI_H

enum wifi_return_status {
    WIFI_STATUS_OK = 0,
    WIFI_STATUS_ERROR = -1,
};

enum wifi_return_status wifi_init_and_connect();

#endif /* WIFI_H */