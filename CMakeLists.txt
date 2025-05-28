cmake_minimum_required(VERSION 3.20)

project(arfchat)

file(GLOB COMMON_SRC "common/*.c")
file(GLOB LIB_SRC "libarfchat/src/*.c")
file(GLOB CLIENT_SRC "arfchat/src/*.c")
file(GLOB DAEMON_SRC "arfchatd/src/*.c")
file(GLOB WS_SRC "arfchat-ws/src/*.c")

include_directories(".")

add_executable(arfchat ${COMMON_SRC} ${LIB_SRC} ${CLIENT_SRC})
add_executable(arfchatd ${COMMON_SRC} ${LIB_SRC} ${DAEMON_SRC})
add_executable(arfchat-ws ${COMMON_SRC} ${LIB_SRC} ${WS_SRC})

target_link_libraries(arfchat-ws websockets cap)

