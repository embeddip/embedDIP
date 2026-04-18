# Architecture profile: Xtensa family

set(EMBEDDIP_ARCH_SOURCES
    arch/xtensa/xtensa_common.cpp
    arch/xtensa/xtensa_fft.cpp
)

set(EMBEDDIP_ARCH_DEFINES
    EMBED_DIP_ARCH_XTENSA=1
)

if(EMBEDDIP_CPU STREQUAL "LX6")
    list(APPEND EMBEDDIP_ARCH_DEFINES EMBED_DIP_CPU_LX6=1)
elseif(EMBEDDIP_CPU STREQUAL "LX7")
    list(APPEND EMBEDDIP_ARCH_DEFINES EMBED_DIP_CPU_LX7=1)
else()
    message(FATAL_ERROR "Unsupported CPU for XTENSA arch: ${EMBEDDIP_CPU}. Supported: LX6, LX7")
endif()

set(EMBEDDIP_ARCH_COMPILE_OPTIONS
)
