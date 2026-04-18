# Architecture profile: ARM family

set(EMBEDDIP_ARCH_SOURCES
    arch/arm/cm7_common.c
    arch/arm/cm7_fft.c
)

set(EMBEDDIP_ARCH_DEFINES
    EMBED_DIP_ARCH_ARM=1
)

if(EMBEDDIP_CPU STREQUAL "CORTEX_M7")
    list(APPEND EMBEDDIP_ARCH_DEFINES EMBED_DIP_CPU_CORTEX_M7=1)
else()
    message(FATAL_ERROR "Unsupported CPU for ARM arch: ${EMBEDDIP_CPU}. Supported: CORTEX_M7")
endif()

list(APPEND EMBEDDIP_ARCH_DEFINES
    ARM_MATH_CM7
)

set(EMBEDDIP_ARCH_PRIVATE_DEFINES
    __FPU_PRESENT=1
)

set(EMBEDDIP_ARCH_COMPILE_OPTIONS
    -mcpu=cortex-m7
    -mfpu=fpv5-sp-d16
    -mfloat-abi=hard
    -mthumb
)
