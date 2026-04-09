// cpu_pause.h
#pragma once

#if defined(_MSC_VER)
#include <intrin.h>
#endif

#if (defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64))
    // x86/x64
#if defined(_MSC_VER)
#define CPU_PAUSE() _mm_pause()
#else
#include <immintrin.h>
#define CPU_PAUSE() _mm_pause()
#endif

#elif (defined(__aarch64__) || defined(__arm__) || defined(_M_ARM) || defined(_M_ARM64))
    // ARM/AArch64
#if defined(_MSC_VER)
    // Windows on ARM
#include <windows.h>
#define CPU_PAUSE() YieldProcessor()
#else
    // GCC/Clang on ARM
#define CPU_PAUSE() __asm__ __volatile__("yield" ::: "memory")
#endif

#else
    // fallback
#include <thread>
#define CPU_PAUSE() std::this_thread::yield()
#endif