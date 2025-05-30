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
*/

#ifndef _ARFCHAT_H
#define _ARFCHAT_H

#include <netinet/in.h>

#include <stdint.h>
#include <stddef.h>

/**
 * Message type
 */
typedef enum {
    TYPE_NOP,   /**< Null message */ 
    TYPE_PING,  /**< Ping request */ 
    TYPE_PONG,  /**< Pong response*/
    TYPE_JOIN,  /**< Join chatroom request */
    TYPE_RMSG   /**< Room message */
} arf_type_t;

/**
 * Header structure
 */
typedef struct __attribute__((packed)) {
    uint32_t    _magic; /**< Magic fixed field */
    uint8_t     type;   /**< Message type @see arf_type_t */
    uint8_t     flags;  /**< Message flag bitfield */
    uint16_t    len;    /**< Data field length */
    uint32_t    s_uid;  /**< Sender uid */
} arf_header_t;

/**
 * \brief Creates arfchat socket
 * Creates a non-blocking UDP socket with SO_REUSEADDR,
 * binds it to INADDR_ANY:ARF_PORT
 * And joins it to the ARF_GROUP multicast group.
 *
 * @param relay_server Relay server IP address string
 *  
 * @return 0 on success, non-zero on error, see errno
 */
int arfchat_init(const char *relay_server);

/**
 * Close arfchat socket
 */
void arfchat_destroy();

/**
 * Receive message header and data
 *
 * @param header Pointer to const arf_header_t* to be set
 * @param data Pointer to const char* to be set
 * @param addr Pointer to a struct sockaddr_in to be set to the sender's address
 * @return Number of bytes received or negative on error, see errno
 */
int arfchat_recv_raw(const arf_header_t **header, const char **data,
    struct sockaddr_in *addr);

/**
 * Send raw buffer unicast
 *
 * @param buff Raw message to send
 * @param size Size of message
 * @param addr Endpoint to send to
 * @return Number of bytes sent or negative on error, see errno
 */
int arfchat_sendto_raw(const void *buff, size_t size, struct sockaddr_in *addr);

/**
 * Send raw buffer multicast
 *
 * @param buff Raw message to send
 * @param size Size of message
 * @return Number of bytes sent or negative on error, see errno
 */
int arfchat_send_raw(const void *buff, size_t size);


int send_ping(uint32_t uid);
int send_pong(uint32_t uid, uint16_t rid, const char *nick, const char *hname,
    const char *rname);
int send_join(uint32_t uid, uint16_t rid, const char *rname);
int send_rmsg(uint32_t uid, uint16_t rid, const char *msg);

#endif /* _ARFCHAT_H */

