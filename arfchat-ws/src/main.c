#include <stdio.h>

#include <libwebsockets.h>

#include "config.h"


static int
ws_service_callback(
    struct lws *wsi,
    enum lws_callback_reasons reason,
    void *user,
    void *in,
    size_t len)
{
    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED:
            printf("connection established\n");
        break;
        case LWS_CALLBACK_RECEIVE:
            printf("received: %.*s\n", len, (char*)in);
        break;
        case LWS_CALLBACK_CLOSED:
            printf("connection closed\n");
        break;
    }

    return 0;
}

struct per_session_data {
    int fd;
};

int
main()
{
    const char *interface = NULL;

    struct lws_protocols protocols[2] = {
        {
            .name =                     "arfchat",
            .callback =                 ws_service_callback,
            .per_session_data_size =    sizeof(struct per_session_data),
            .rx_buffer_size =           0
        },
        {
            .name =                     NULL
        }
    };

    struct lws_context_creation_info info = {
        .port =         WS_PORT,
        .iface =        interface,
        .protocols =    protocols,
        .gid =          -1,
        .uid =          -1
    };

    struct lws_context *context = lws_create_context(&info);
    if (context == NULL) {
        fprintf(stderr, "error lws_create_context\n");
        return -1;
    }

    while (1) {
        lws_service(context, 50);
    }

    return 0;
}

