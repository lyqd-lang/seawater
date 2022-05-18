#pragma once

/* 
Prefer clang over other compilers due to it being the
most annoying to work with when it comes to type definign and redefining
*/

#ifndef __GNUC__
    /* Compilation with GCC/G++ on Linux */
    #define uint8_t unsigned char
    #define uint32_t unsigned long int
    #define uint64_t long long unsigned int

    #define lqdchar char
#else
    #ifdef __MINGW32__
        /* Compilation with GCC/G++ on Windows */
        #define uint8_t unsigned char
        #define uint32_t unsigned long int
        #define uint64_t long long unsigned int

        #define lqdchar char
    #else
        /* Compilation with clang or MSVC */
        #include <stdint.h>
        #define lqdchar short
    #endif
#endif