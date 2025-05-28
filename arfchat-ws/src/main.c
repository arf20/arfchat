/*
    arfchat: Local and LAN chat application
    Copyright (C) 2024 Angel Ruiz Fernandez <arf20>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
    
    main.c: Application entry point
*/

#include <stdio.h>

#include <libwebsockets.h>

#include "common/config.h"


static int
ws_service_callback(
    struct lws *wsi,
    enum lws_callback_reasons reason,
    void *user,
    void *in,
    size_t len)
{
    char send_buff[LWS_PRE + 65535];
    char *send_block = &send_buff[LWS_PRE];

    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED:
            printf("connection established\n");
        break;
        case LWS_CALLBACK_RECEIVE:
            printf("received: %.*s\n", len, (char*)in);
            size_t len = snprintf(send_block, 65535, "ok\n");
            lws_write(wsi, send_block, len, LWS_WRITE_BINARY);
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
        .port =         ARF_WS_PORT,
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

