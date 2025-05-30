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

    arfchat.c: Library definitions
*/

#include "../include/arfchat.h"

#include "../../common/config.h"

#include <stdio.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

/**
 * BSD compatibility for SO_REUSEPORT
 */
#ifdef __linux__
    #define COMPAT_REUSE    SO_REUSEADDR
#elif  __unix__
    #define COMPAT_REUSE    SO_REUSEPORT
#endif


/**
 * Socket file descriptor
 */
static int fd = -1;

/**
 * Send/receive buffer
 */
static char buff[ARF_BUFF_SIZE];

/**
 * Multicast group destination for messages
 */
static struct sockaddr_in group_addr;

/**
 * Optional unicast relay server address
 */
static struct sockaddr_in relay_addr;



int
arfchat_init(const char *relay_server)
{
    /* Create socket */
    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd < 0) return -1;

    /* Allow multiple sockets to use the same port on the same interface */
    unsigned int yes = 1;
    if (setsockopt(fd, SOL_SOCKET, COMPAT_REUSE, &yes, sizeof(yes)) < 0)
       return -1;

    /* Bind address */
    struct sockaddr_in addr;
    memset(&group_addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(ARF_PORT);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        return -1;
    
    /* Join socket to multicast group */
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(ARF_GROUP);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*) &mreq,
        sizeof(mreq)) < 0)
    {
        return -1;
    }

    /* Set non-blocking with fcntl for cross-compatibility */
    if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK) < 0)
        return -1;

    /* Set group address */
    memset(&group_addr, 0, sizeof(addr));
    group_addr.sin_family = AF_INET;
    group_addr.sin_addr.s_addr = inet_addr(ARF_GROUP);
    group_addr.sin_port = htons(ARF_PORT);


    /* If set, set relay server address */
    if (!relay_server)
        return 0;

    memset(&relay_addr, 0, sizeof(struct sockaddr_in));
    if (relay_server) {
        relay_addr.sin_family = AF_INET;
        relay_addr.sin_addr.s_addr = inet_addr(relay_server);
        relay_addr.sin_port = htons(ARF_PORT);
    }

    return 0;
}

void
arfchat_destroy()
{
    close(fd);
}

int
arfchat_recv_raw(const arf_header_t **header, const char **data,
    struct sockaddr_in *addr)
{
    socklen_t addrlen = sizeof(addr);
    int r = recvfrom(fd, buff, ARF_BUFF_SIZE, 0, (struct sockaddr*)addr, &addrlen);

    *header = (arf_header_t*)buff;
    *data = buff + sizeof(arf_header_t);

    return r;
}

int
arfchat_sendto_raw(const void *buff, size_t size, struct sockaddr_in *addr)
{
    return sendto(fd, buff, size, 0, (struct sockaddr*)addr,
        sizeof(struct sockaddr_in));
}

int
arfchat_send_raw(const void *buff, size_t size)
{
    return sendto(fd, buff, size, 0, (struct sockaddr*)&group_addr,
        sizeof(struct sockaddr_in));
}


int
send_ping(uint32_t uid)
{
    arf_header_t header = { 0 };
    header._magic = ARF_MAGIC;
    header.type = TYPE_PING;
    header.s_uid = uid;

    int r = sendto(fd, &header, sizeof(header), 0, (struct sockaddr*)&group_addr,
            sizeof(struct sockaddr));
    if (r != 0 && relay_addr.sin_family != 0)
        r = sendto(fd, &header, sizeof(header), 0,
            (struct sockaddr*)&relay_addr, sizeof(struct sockaddr));
    return r;
}

int
send_pong(uint32_t uid, uint16_t rid, const char *nick,
    const char *hname, const char *rname)
{
    arf_header_t *header = (arf_header_t*)buff;
    header->_magic = ARF_MAGIC;
    header->type = TYPE_PONG;
    header->s_uid = uid;

    char *data = buff + sizeof(arf_header_t);
    int datalen = 0;
    
    *(uint16_t*)data = rid;
    datalen += 2;

    datalen += 2; /* padd? */

    strcpy(data + datalen, nick);
    datalen += strlen(nick) + 1;

    strcpy(data + datalen, hname);
    datalen += strlen(hname) + 1;

    if (rname != NULL) {
        strcpy(data + datalen, rname);
        datalen += strlen(rname) + 1;
    } else {
        strcpy(data + datalen, "");
        datalen++;
    }

    int r = sendto(fd, buff, sizeof(arf_header_t) + datalen, 0,
            (struct sockaddr*)&group_addr, sizeof(struct sockaddr));
    if (r != 0 && relay_addr.sin_family != 0)
        r = sendto(fd, buff, sizeof(arf_header_t) + datalen, 0,
            (struct sockaddr*)&relay_addr, sizeof(struct sockaddr));
    return r;
}

int
send_join(uint32_t uid, uint16_t rid, const char *rname)
{
    arf_header_t *header = (arf_header_t*)buff;
    header->_magic = ARF_MAGIC;
    header->type = TYPE_JOIN;
    header->s_uid = uid;

    char *data = buff + sizeof(arf_header_t);
    int datalen = 0;
    
    *(uint16_t*)data = rid;
    datalen += 2;

    datalen += 2; /* padd? */

    strcpy(data + datalen, rname);
    datalen += strlen(rname) + 1;

    int r = sendto(fd, buff, sizeof(arf_header_t) + datalen, 0,
            (struct sockaddr*)&group_addr, sizeof(struct sockaddr));
    if (r != 0 && relay_addr.sin_family != 0)
        r = sendto(fd, buff, sizeof(arf_header_t) + datalen, 0,
            (struct sockaddr*)&relay_addr, sizeof(struct sockaddr));
    return r;

}

int
send_rmsg(uint32_t uid, uint16_t rid, const char *msg)
{
    arf_header_t *header = (arf_header_t*)buff;
    header->_magic = ARF_MAGIC;
    header->type = TYPE_RMSG;
    header->s_uid = uid;

    char *data = buff + sizeof(arf_header_t);
    int datalen = 0;
    
    *(uint16_t*)data = rid;
    datalen += 2;

    datalen += 2; /* padd? */

    strcpy(data + datalen, msg);
    datalen += strlen(msg) + 1;

    int r = sendto(fd, buff, sizeof(arf_header_t) + datalen, 0,
        (struct sockaddr*)&group_addr, sizeof(struct sockaddr));
    if (r != 0 && relay_addr.sin_family != 0)
        r = sendto(fd, buff, sizeof(arf_header_t) + datalen, 0,
            (struct sockaddr*)&relay_addr, sizeof(struct sockaddr));
    return r;
}


