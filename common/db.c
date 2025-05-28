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

    db.c: Local tracking of users and rooms
*/

#include "db.h"

#include <stdlib.h>
#include <string.h>

void
user_list_push(user_node_t *l, uint32_t uid, const char *nick, uint16_t rid,
    struct sockaddr_in addr, const char *hname)
{
    for (user_node_t *i = l->next; i != NULL; i = i->next)
        if (i->uid == uid) {
            free(i->nick);
            i->nick = strdup(nick);
            i->rid = rid;
            i->addr = addr;
            free(i->hname);
            i->hname = strdup(hname);
            return;
        }
    
    /* Last node */
    user_node_t *i = l;
    while (i->next) { i = i->next; };

    /* Create node */
    i->next = malloc(sizeof(user_node_t));
    i->next->uid = uid;
    i->next->nick = strdup(nick);
    i->next->rid = rid;
    i->next->addr = addr;
    i->next->hname = strdup(hname);
    i->next->next = NULL;
}

void
user_list_set_rid(user_node_t *l, uint32_t uid, uint16_t rid)
{
    for (user_node_t *i = l->next; i != NULL; i = i->next)
        if (i->uid == uid) {
            i->rid = rid;
            return;
        }
}

const char *
user_list_get_nick(user_node_t *l, uint32_t uid)
{
    for (user_node_t *i = l->next; i != NULL; i = i->next)
        if (i->uid == uid)
            return i->nick;
    return NULL;
}

uint16_t
user_list_get_rid(user_node_t *l, uint32_t uid)
{
    for (user_node_t *i = l->next; i != NULL; i = i->next)
        if (i->uid == uid)
            return i->rid;
    return 0;
}

void
user_list_remove(user_node_t *l, uint32_t uid) 
{
    for (user_node_t *i = l->next; i->next != NULL; i = i->next) {
        if (i->next->uid == uid) {
            user_node_t *t = i->next;
            i->next = i->next->next;
            free(t->hname);
            free(t->nick);
            free(t);
            return;
        }
    }
}

void
user_list_destroy(user_node_t *l)
{
    for (user_node_t *i = l->next; i != NULL;) {
        user_node_t *t = i->next;
        free(i->nick);
        free(i->hname);
        free(i);
        i = t;
    }
    free(l);
}

void
room_list_push(room_node_t *l, uint16_t rid, const char *rname)
{
    for (room_node_t *i = l->next; i != NULL; i = i->next)
        if (i->rid == rid) {
            free(i->rname);
            i->rname = strdup(rname);
            return;
        }
    
    /* Last node */
    room_node_t *i = l;
    while (i->next) { i = i->next; };

    /* Create node */
    i->next = malloc(sizeof(room_node_t));
        i->next->rid = rid;
    i->next->rname = strdup(rname);
    i->next->next = NULL;
}

const char *
room_list_get_rname(room_node_t *l, uint16_t rid)
{
    for (room_node_t *i = l->next; i != NULL; i = i->next)
        if (i->rid == rid)
            return i->rname;
    return NULL;
}

void
room_list_clean_empty(room_node_t *rl, user_node_t *ul)
{
    for (room_node_t *i = rl->next; i->next != NULL; i = i->next) {
        int found = 0;
        for (user_node_t *j = ul->next; j != NULL; j = j->next)
            if (j->rid == i->next->rid)
                found = 1;

        if (!found) {
            room_node_t *t = i->next;
            i->next = i->next->next;
            free(t->rname);
            free(t);
        }
    }
}

void
room_list_destroy(room_node_t *l)
{
    for (room_node_t *i = l->next; i != NULL;) {
        room_node_t *t = i->next;
        free(i->rname);
        free(i);
        i = t;
    }
    free(l);
}

