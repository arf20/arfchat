# archat
Local and LAN chat application

## Functionality

arfchat uses a multicast protocol to communicate with any and all clients in the
same network described in doc/PROTOCOL.md

Right now there is support for:
 - nicknames
 - joining chatrooms
 - sending and receiving messages (yikes)
 - relaying messages between networks (debatible)



## Project structure

 - arfchat:         local P2P client source
 - arfchatd:        internetwork relay daemon source
 - libarfchat:      protocol de/serialization library
 - common:          common code
 - doc:             protocol spec & documentation
 - ws\_dissect.lua: protocol description file for WireShark inspection

## Build

The universal CMake thing

```
mkdir build && cd build
cmake ..
make
```

Tested in Linux and OpenBSD.

BSD note: because this application reuses address and port, to launch more than
once instance of arfchat (client), all instances have to run as the same user
because the BSD kernel has that as a security feature (this is undocumented behaviour).
To allow multiple different users to chat on the same BSD system, the sysop must
install arfchat system wide, owned by a system user such as `\_arfchat` and have
setuid bit enabled to set its euid to said user. This way the getlogin() functionality
is preserved, launched by the different users, and effectively running as the same user.
This is confirmed to be also the case in at least FreeBSD and NetBSD, it appears to be
a geberal BSD kernel security thing which I didn't know about.

