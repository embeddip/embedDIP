# Architecture profile: native host

set(EMBEDDIP_ARCH_SOURCES
    arch/host/host_timer.c
    arch/host/host_fft.c
)

set(EMBEDDIP_ARCH_DEFINES
    EMBED_DIP_ARCH_HOST=1
    EMBED_DIP_CPU_NATIVE=1
)

set(EMBEDDIP_ARCH_COMPILE_OPTIONS)
