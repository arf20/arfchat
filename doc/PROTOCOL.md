# ACP

arf chat protocol

## Header

Size: 12 bytes

```
  4 bytes
+-------------------------------+
| MAGIC NUMBER                  |
+-------+-------+-------+-------+
| TYPE  | FLAGS | DATA LENGTH   |
+-------+-------+---------------+
| SOURCE UID                    |
+-------------------------------+
| DATA                          |
| ...                           |

```

Magic number: 0x6942

## Packet types

### NOP  0x00
No operation. No data.

### PING 0x01
Autodiscovery request. No data.

### PONG 0x02
Autodiscovery reply.

Discovers users and rooms. Provides uid, rid, nickname and room name.

Data:
```
+---------------+---------------+
| RID           | reserved      |
+---------------+---------------+
| Nickname NUL-terminated       |
| ...                           |
+-------------------------------+
| Hostname                      |
| ...                           |
+-------------------------------+
| Room name NUL-terminated      |
| ...                           |
+-------------------------------+
```

### JOIN 0x03
Room join.

Used to create rooms, join existing rooms and change room names.

Data:
```
+---------------+---------------+
| RID           |               |
+---------------+---------------+
| Room name NUL-terminated      |
| ...                           |
```

### RMSG 0x04
Room message

Data:
```
+---------------+---------------+
| RID           |               |
+---------------+---------------+
| Message NUL-terminated        |
| ...                           |
```

## Fields

- UID: User ID, 32-bit unsigned
- RID: Room ID, 16-bit unsigned


