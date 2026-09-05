# Board profile: native host

set(EMBEDDIP_BOARD_SOURCES
    ${BOARD_COMMON_SOURCES}
    board/host/board_host_memory.c
)

set(EMBEDDIP_DEVICE_SOURCES)

set(EMBEDDIP_BOARD_DEFINES
    EMBED_DIP_BOARD_HOST=1
)
