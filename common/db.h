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

#ifndef _DB_H
#define _DB_H

#include <stdint.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>

typedef struct user_node_s {
    uint32_t uid;
    char *nick;
    uint16_t rid;
    struct sockaddr_in addr;
    char *hname;

    struct user_node_s *next;
} user_node_t;

typedef struct room_node_s {
    uint16_t rid;
    char *rname;
    
    struct room_node_s *next;
} room_node_t;

void user_list_push(user_node_t *l, uint32_t uid, const char *nick,
    uint16_t rid, struct sockaddr_in addr, const char *hname);
void user_list_set_rid(user_node_t *l, uint32_t uid, uint16_t rid);
const char *user_list_get_nick(user_node_t *l, uint32_t uid);
uint16_t user_list_get_rid(user_node_t *l, uint32_t uid);
void user_list_remove(user_node_t *l, uint32_t uid);
void user_list_destroy(user_node_t *l);

void room_list_push(room_node_t *l, uint16_t rid, const char *rname);
const char *room_list_get_rname(room_node_t *l, uint16_t rid);
void room_list_clean_empty(room_node_t *rl, user_node_t *ul);
void room_list_destroy(room_node_t *l);

#endif /* _DB_H */
