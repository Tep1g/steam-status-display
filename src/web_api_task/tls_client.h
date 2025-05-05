#ifndef TLS_CLIENT_H
#define TLS_CLIENT_H

enum tls_return_status {
    TLS_STATUS_OK = 0,
    TLS_STATUS_ERROR = -1,
};

enum tls_return_status tls_client_init();

#endif /* TLS_CLIENT_H */