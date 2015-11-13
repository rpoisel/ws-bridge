project("ws-bridge" C)
cmake_minimum_required(VERSION 3.0)

SET(LIBWEBSOCKETS_INSTALLDIR "" CACHE PATH "Installation directory of libwebsockets")

include_directories(
    ${LIBWEBSOCKETS_INSTALLDIR}/include
    )

link_directories(
    ${LIBWEBSOCKETS_INSTALLDIR}/lib
    )

add_executable(
    ws-bridge
    src/main.c
    )

target_link_libraries(
    ws-bridge
    websockets
    )

