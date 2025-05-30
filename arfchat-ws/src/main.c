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

#include "libarfchat/include/arfchat.h"
#include "ws.h"
#include "common/config.h"


int
main()
{
    printf("arfchat-ws  Copyright (C) 2024  Angel Ruiz Fernandez <arf20>\n"
        "This program comes with ABSOLUTELY NO WARRANTY\n"
        "This is free software, and you are welcome to redistribute it\n"
        "under certain conditions.\n\n");

    /* Init */
    if (arfchat_init(NULL) < 0) {
        printf("arfchat_init: %s\n", strerror(errno));
        return 1;
    }

    struct lws_context *context = NULL;
    if (!(context = ws_init())) {
        printf("ws_init: %s\n", strerror(errno));
        return 1;
    }

    printf("listening on %d\n", ARF_WS_PORT);


    ws_run(context);

    return 0;
}

