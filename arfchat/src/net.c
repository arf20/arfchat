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

    net.c: Networking functions
*/

#include "net.h"

#include <stdio.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#ifdef __linux__
    #define COMPAT_REUSE    SO_REUSEADDR
#elif  __unix__
    #define COMPAT_REUSE    SO_REUSEPORT
#endif

static int fd = 0;
static char buff[2048];

static struct sockaddr_in dest_addr;
static struct sockaddr_in relay_addr;

int
create_sockets(const char *relay_server)
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
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        return -1;

    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(GROUP);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*) &mreq,
        sizeof(mreq)) < 0)
    {
        return -1;
    }

    /* Set non-blocking with fcntl for cross-compatibility */
    if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK) < 0)
        return -1;

    /* Set multicast destination address */
    memset(&dest_addr, 0, sizeof(struct sockaddr_in));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr(GROUP);
    dest_addr.sin_port = htons(PORT);

    /* Set relay destination address if any */
    memset(&relay_addr, 0, sizeof(struct sockaddr_in));
    if (relay_server) {
        relay_addr.sin_family = AF_INET;
        relay_addr.sin_addr.s_addr = inet_addr(relay_server);
        relay_addr.sin_port = htons(PORT);
    }

    return 0;
}

void
destroy_sockets()
{
    close(fd);
}

int
recv_message(const header_t **header, const char **data, struct sockaddr_in *addr)
{
    socklen_t addrlen = sizeof(addr);
    int r = recvfrom(fd, buff, 2048, 0, (struct sockaddr*)addr, &addrlen);

    *header = (header_t*)buff;
    *data = buff + sizeof(header_t);

    return r;
}

int
send_ping(uint32_t uid)
{
    header_t header = { 0 };
    header._magic = MAGIC;
    header.type = TYPE_PING;
    header.s_uid = uid;

    int r = sendto(fd, &header, sizeof(header), 0, (struct sockaddr*)&dest_addr,
            sizeof(struct sockaddr));
    if (r != 0 && relay_addr.sin_family != 0)
        r = sendto(fd, &header, sizeof(header), 0, (struct sockaddr*)&relay_addr,
            sizeof(struct sockaddr));
    return r;
}

int
send_pong(uint32_t uid, int16_t rid, const char *nick,
    const char *hname, const char *rname)
{
    header_t *header = (header_t*)buff;
    header->_magic = MAGIC;
    header->type = TYPE_PONG;
    header->s_uid = uid;

    char *data = buff + sizeof(header_t);
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

    int r = sendto(fd, buff, sizeof(header_t) + datalen, 0,
            (struct sockaddr*)&dest_addr, sizeof(struct sockaddr));
    if (r != 0 && relay_addr.sin_family != 0)
        r = sendto(fd, buff, sizeof(header_t) + datalen, 0,
            (struct sockaddr*)&relay_addr, sizeof(struct sockaddr));
    return r;
}

int
send_join(uint32_t uid, uint16_t rid, const char *rname)
{
    header_t *header = (header_t*)buff;
    header->_magic = MAGIC;
    header->type = TYPE_JOIN;
    header->s_uid = uid;

    char *data = buff + sizeof(header_t);
    int datalen = 0;
    
    *(uint16_t*)data = rid;
    datalen += 2;

    datalen += 2; /* padd? */

    strcpy(data + datalen, rname);
    datalen += strlen(rname) + 1;

    int r = sendto(fd, buff, sizeof(header_t) + datalen, 0,
            (struct sockaddr*)&dest_addr, sizeof(struct sockaddr));
    if (r != 0 && relay_addr.sin_family != 0)
        r = sendto(fd, buff, sizeof(header_t) + datalen, 0,
            (struct sockaddr*)&relay_addr, sizeof(struct sockaddr));
    return r;

}

int
send_rmsg(uint32_t uid, uint16_t rid, const char *msg)
{
    header_t *header = (header_t*)buff;
    header->_magic = MAGIC;
    header->type = TYPE_RMSG;
    header->s_uid = uid;

    char *data = buff + sizeof(header_t);
    int datalen = 0;
    
    *(uint16_t*)data = rid;
    datalen += 2;

    datalen += 2; /* padd? */

    strcpy(data + datalen, msg);
    datalen += strlen(msg) + 1;

    int r = sendto(fd, buff, sizeof(header_t) + datalen, 0,
        (struct sockaddr*)&dest_addr, sizeof(struct sockaddr));
    if (r != 0 && relay_addr.sin_family != 0)
        r = sendto(fd, buff, sizeof(header_t) + datalen, 0,
            (struct sockaddr*)&relay_addr, sizeof(struct sockaddr));
    return r;
}
