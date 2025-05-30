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
    
    ws.c: WebSocket server
*/

#include "ws.h"

#include "common/config.h"

#include "libarfchat/include/arfchat.h"

struct per_session_data {
    int fd;
};


static int
ws_service_callback(
    struct lws *wsi,
    enum lws_callback_reasons reason,
    void *user,
    void *in,
    size_t len)
{
    char send_buff[LWS_PRE + ARF_BUFF_SIZE];
    char *send_block = &send_buff[LWS_PRE];

    /* handle arfchat events */
    const arf_header_t *header;
    const char *data;
    struct sockaddr_in s_addr;
    int recvsize = 0;
    if ((recvsize = arfchat_recv_raw(&header, &data, &s_addr)) < 0) {
        if (errno != EAGAIN) {
            fprintf(stderr, "arfchat_recv_raw: %s\n", strerror(errno));
            return -1;
        }
    } else {
        /* relay message */
        printf("received %d from arfchat\n", recvsize);
        lws_write(wsi, (char*)header, recvsize, LWS_WRITE_BINARY);
    }


    /* handle websocket events */
    char namebuf[256];
    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED:
            lws_get_peer_simple(wsi, namebuf, 256);
            printf("connection from %s\n", namebuf);
        break;
        case LWS_CALLBACK_RECEIVE:
            printf("received %d from ws\n", len);
            arfchat_send_raw(in, len);
        break;
        case LWS_CALLBACK_CLOSED:
            printf("connection closed\n");
        break;
        default:
            return 0;
    }

    return 0;
}

struct lws_context*
ws_init()
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
        return NULL;
    }

    return context;
}

void
ws_run(struct lws_context *context)
{
    while (1) {
        lws_service(context, 12);
    }
}


