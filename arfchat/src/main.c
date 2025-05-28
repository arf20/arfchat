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
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include "common/db.h"
#include "libarfchat/include/arfchat.h"


struct termios saved_tattr = { };

void
prompt_input(char *buff, size_t n)
{
    char c;
    int i = 0;
    while ((c = getchar()) != '\n' && i < n - 2) {
        if (c == -1) continue;
        if (c == 127) {
            i--;
            printf("\b \b");
            continue;
        }
        putchar(c);
        buff[i] = c;
        i++;
    }
    buff[i] = '\0';
    putchar('\n');
}

void
usage(const char *self)
{
    printf("%s: [-d] [server]\n\n\t-d\tDebug\n\trelay\tinternet relay server\n");
}

int
main(int argc, char **argv)
{
    printf("arfchat  Copyright (C) 2024  Angel Ruiz Fernandez <arf20>\n"
        "This program comes with ABSOLUTELY NO WARRANTY\n"
        "This is free software, and you are welcome to redistribute it\n"
        "under certain conditions.\n\n");

    srand(time(NULL));

    if (argc > 3) {
        usage(argv[0]);
        return 1;
    }

    int debug = 0;
    char *relay_server = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0)
            debug = 1;
        else
            relay_server = argv[i]; 
    }

    /* Enviroment information */
    char hname[1024];
    gethostname(hname, 1024);

    /* User information */
    uint32_t uid = rand();
    char *nick = getlogin();
    char *nickbuff = NULL;
    size_t nicklen = 0;

    /* Room information */
    uint16_t rid = 0;
    char *rname = NULL;

    /* DB */
    user_node_t *user_list = malloc(sizeof(user_node_t));
    user_list->next = NULL;
    room_node_t *room_list = malloc(sizeof(room_node_t));
    room_list->next = NULL;

    printf("Nickname [%s]> ", nick);
    getline(&nickbuff, &nicklen, stdin);
    nickbuff[strlen(nickbuff) - 1] = '\0';

    if (strlen(nickbuff) > 0) nick = nickbuff;

    if (debug)
        printf("==uid: %d\n==nick: %s\n", uid, nick);

    /* Init */
    if (arfchat_init(relay_server) < 0) {
        printf("arfchat_init: %s\n", strerror(errno));
        return 1;
    }

    /* Begin autodiscovery process */
    if (debug)
        printf(">>PING %d\n", uid);
    if (send_ping(uid) < 0) {
        printf("send_ping: %s\n", strerror(errno));
        return 1;
    }

    time_t t_begin = time(NULL);

    /* Non-canonical console */
    struct termios tattr = { };
    tcgetattr(STDIN_FILENO, &tattr);
    saved_tattr = tattr;
    tattr.c_lflag &= ~(ICANON|ECHO);
    tattr.c_oflag |= ONLCR;
    tattr.c_cc[VMIN] = 1;
    tattr.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &tattr);

    /* Non-blocking console */
    fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);

    int run = 1;
    const arf_header_t *header;
    const char *data;
    struct sockaddr_in s_addr;
    while (run) {
        if (arfchat_recv_raw(&header, &data, &s_addr) < 0) {
            if (errno != EAGAIN) {
                printf("recv_message: %s\n", strerror(errno));
                break;
            }
        } else {
            /* Handle incoming packets */
            switch (header->type) {
                case TYPE_PING: {
                    if (debug) {
                        printf("<<PING %d\n", header->s_uid);
                        printf(">>PONG %d %d %s %s %s\n", uid, rid, nick,
                            hname, rname);
                    }

                    send_pong(uid, rid, nick, hname, rname);
                } break;
                case TYPE_PONG: {
                    if (debug)
                        printf("<<PONG %d ", header->s_uid);

                    uint16_t s_rid = *(uint16_t*)data;
                    const char *s_nick = data + 4;
                    const char *s_hname = s_nick + strlen(s_nick) + 1;
                    const char *s_rname = s_hname + strlen(s_hname) + 1;
                    
                    if (debug)
                        printf("%d %s %s %s\n", s_rid, s_nick, s_hname, s_rname);

                    user_list_push(user_list, header->s_uid, s_nick,
                        s_rid, s_addr, s_hname);
                    if (s_rid != 0)
                        room_list_push(room_list, s_rid, s_rname);
                } break;
                case TYPE_JOIN: {
                    if (debug)
                        printf("<<JOIN %d ", header->s_uid);

                    uint16_t s_rid = *(uint16_t*)data;
                    const char *s_rname = data + 4;

                    if (debug)
                        printf("%d %s\n", s_rid, s_rname);

                    uint16_t prev_rid = user_list_get_rid(user_list,+
                        header->s_uid);
                    
                    if ((prev_rid == rid) && (prev_rid != s_rid))
                        printf("%s has left %s\n", user_list_get_nick(user_list,
                            header->s_uid), rname);

                    user_list_set_rid(user_list, header->s_uid, s_rid);
                    room_list_push(room_list, s_rid, s_rname);

                    if ((s_rid == rid) && (header->s_uid != uid))
                        printf("%s has joined %s\n", user_list_get_nick(user_list,
                            header->s_uid), rname);
                } break;
                case TYPE_RMSG: {
                    if (debug)
                        printf("<<RMSG %d ", header->s_uid);

                    uint16_t s_rid = *(uint16_t*)data;
                    const char *s_msg = data + 4;

                    if (debug)
                        printf("%d %s\n", s_rid, s_msg);

                    if ((s_rid == rid) && (header->s_uid != uid))
                        printf("[%s] %s\n",
                            user_list_get_nick(user_list, header->s_uid),
                            s_msg);
                } break;
            }
        }

        /* Handle commands */
        char cmd;
        if (read(0, &cmd, 1) < 0) {
            if (errno != EAGAIN) {
                printf("read: %s\n", strerror(errno));
                break;
            }
        } else  {
            switch (cmd) {
                case 'q': {
                    if (debug)
                        printf(">>JOIN %d %d %s\n", uid, 0, "");
                    send_join(uid, 0, "");
                    run = 0;
                }; break;
                case 'n': {
                    printf("\n user-name@host-name               from      in-room\n");
                    printf(  "-----------------------------------------------------\n");
                    for (user_node_t *i = user_list->next; i != NULL; i = i->next)
                        printf("%10s@%-10s   %15s   %10s\n", i->nick, i->hname,
                            inet_ntoa(i->addr.sin_addr),
                            room_list_get_rname(room_list, i->rid));
                    puts(    "-----------------------------------------------------\n\n");
                } break;
                case 'w': {
                    int c = 0;
                    for (user_node_t *i = user_list->next; i != NULL; i = i->next)
                        if (i->rid == rid) c++;
                    printf("\n[you are in '%s' among %d]\n\n", rname, c);

                    for (user_node_t *i = user_list->next; i != NULL; i = i->next)
                        if (i->rid == rid) printf("%s ", i->nick);
                    puts("\n\n");
                } break;
                case 'l': {
                    int maxw = 0;
                    for (room_node_t *i = room_list->next; i != NULL; i = i->next) {
                        int w = strlen(i->rname);
                        if (w > maxw) maxw = w;
                    }

                    printf("\n room name   #\n");
                    printf("---------------\n");
                    for (room_node_t *i = room_list->next; i != NULL; i = i->next) {
                        int c = 0;
                        for (user_node_t *j = user_list->next; j != NULL; j = j->next)
                            if (i->rid == j->rid) c++;
                        printf("%10s %3d\n", i->rname, c);
                    }
                    printf("---------------\n\n");
                } break;
                case 'j': {
                    printf(":join> ");
                    char joinrname[256];
                    prompt_input(joinrname, 256);

                    room_node_t *found = NULL;
                    for (room_node_t *i = room_list->next; i != NULL; i = i->next) {
                        if (strcmp(i->rname, joinrname) == 0) {
                            found = i;
                            break;
                        }
                    }

                    if (found) {
                        /* Join room */
                        if (debug)
                            printf(">>JOIN %d %d %s\n", uid, found->rid, found->rname);
                        send_join(uid, found->rid, found->rname);

                        rid = found->rid;
                        rname = strdup(found->rname);
                    } else {
                        /* Create room */
                        uint16_t new_rid = 1 + (rand() % 65534);

                        if (debug)
                            printf(">>JOIN* %d %d %s\n", uid, new_rid, joinrname);
                        send_join(uid, new_rid, joinrname);

                        rid = new_rid;
                        rname = strdup(joinrname);

                        if (debug)
                            printf("==rid: %d\n==rname: %s\n", rid, rname);
                    }
                } break;
                case ' ': {
                    printf("[%s] ", nick);
                    char msg[1024];
                    prompt_input(msg, 1024);

                    if (debug)
                        printf(">>RMSG %d %d %s\n", uid, rid, msg);
                    send_rmsg(uid, rid, msg);

                } break;
                case 'h': {
                    printf("\n  l - list open rooms\n  w - who is in the room\n"
                        "  j - join a room\n  n - discovered users\n"
                        "  q - quit arfchat\n"
                        "\nTo begin TALK MODE, press [SPACE]\n\n");
                } break;
            }
        }

        usleep(1000);
    }

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_tattr);


    /* Deinit */
    arfchat_destroy();

    for (user_node_t *i = user_list->next; i != NULL;) {
        user_node_t *t = i->next;
        free(i->nick);
        free(i->hname);
        free(i);
        i = t;
    }
    free(user_list);

    for (room_node_t *i = room_list->next; i != NULL;) {
        room_node_t *t = i->next;
        free(i->rname);
        free(i);
        i = t;
    }
    free(room_list);

    free(nickbuff);
    free(rname);

    return 0;
}
