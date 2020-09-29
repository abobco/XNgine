#pragma once

// __ guards prevent compiler warnings in strict ANSI mode
#define ENDIAN_SWAP(val) \
    __asm__ __volatile__ ( \
        "eor     r3, %1, %1, ror #16\n\t" \
        "bic     r3, r3, #0x00FF0000\n\t" \
        "mov     %0, %1, ror #8\n\t" \
        "eor     %0, %0, r3, lsr #8" \
        : "=r" (val) \
        : "0"(val) \
        : "r3", "cc" \
    );