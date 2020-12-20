/*==============================================================================
 * Copyright (C) 2020 YaoYuan <ibireme@gmail.com>.
 * Released under the MIT license (MIT).
 *============================================================================*/

#ifndef yybench_time_h
#define yybench_time_h

#include "yybench_def.h"

#ifdef __cplusplus
extern "C" {
#endif


/*==============================================================================
 * Timer
 *============================================================================*/

/** Structure holding a timestamp. */
typedef struct yy_time yy_time;

/** Get current wall time to a structure. */
static yy_inline void yy_time_get_current(yy_time *t);

/** Convert time structure to seconds. */
static yy_inline f64 yy_time_to_seconds(yy_time *t);

/** Get current wall time in seconds. */
static yy_inline f64 yy_time_get_seconds(void);

/** A high-resolution, low-overhead, fixed-frequency timer for benchmark. */
static yy_inline u64 yy_time_get_ticks(void);


/*==============================================================================
 * Timer (Private)
 *============================================================================*/

#if defined(_WIN32)

struct yy_time {
    LARGE_INTEGER counter;
};

static yy_inline void yy_time_get_current(yy_time *t) {
    QueryPerformanceCounter(&t->counter);
}

static yy_inline f64 yy_time_to_seconds(yy_time *t) {
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    return (f64)t->counter.QuadPart / (f64)freq.QuadPart;
}

#else

struct yy_time {
    struct timeval now;
};

static yy_inline void yy_time_get_current(yy_time *t) {
    gettimeofday(&t->now, NULL);
}

static yy_inline f64 yy_time_to_seconds(yy_time *t) {
    return (f64)t->now.tv_sec + (f64)t->now.tv_usec / 1000.0 / 1000.0;
}

#endif

static yy_inline f64 yy_time_get_seconds(void) {
    yy_time t;
    yy_time_get_current(&t);
    return yy_time_to_seconds(&t);
}

#if defined(_WIN32) && (defined(_M_IX86) || defined(_M_AMD64))
#    pragma intrinsic(__rdtsc)
#endif

static yy_inline u64 yy_time_get_ticks() {
    /*
     RDTSC is a fixed-frequency timer on modern x86 CPU,
     and may not match to CPU clock cycles.
     */
#if defined(_WIN32)
#   if defined(_M_IX86) || defined(_M_AMD64)
    return __rdtsc();
#   else
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (u64)now.QuadPart;
#   endif
    
#elif defined(__i386__) || defined(__i386)
    u64 tsc;
    __asm volatile("rdtsc" : "=a"(tsc));
    return tsc;
    
#elif defined(__x86_64__) || defined(__x86_64) || \
defined(__amd64__) || defined(__amd64)
    u64 lo, hi;
    __asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return (hi << 32u) | lo;
    
#elif defined(__ia64__)
    u64 tsc;
    __asm volatile("mov %0 = ar.itc" : "=r"(tsc));
    return tsc;
    
    /*
     We can use several counter registers in the ARM CPU:
     
     CNTPCT_EL0: physical counter
     CNTVCT_EL0: virtual counter (physical counter - offset)
     PMCCNTR_EL0: performance monitors control cycle count register
     
     The physical counter (and virtual counter) runs at a fixed frequency which
     is different with the CPU cycle rate. For example: Apple A10 max clock rate
     is 2.34GHz, but the physical counter frequency is fixed to 24MHz.
     
     Some of these registers are optional, or may be disabled in user mode.
     For example: read CNTVCT_EL0 or PMCCNTR_EL0 in iPhone may get
     'EXC_BAD_INSTRUCTION' exception.
     */
#elif defined(__aarch64__)
    u64 tsc;
#   if defined(__APPLE__)
    /* used by mach_absolute_time(), see mach_absolute_time.s */
    __asm volatile("mrs %0, cntpct_el0" : "=r"(tsc));
#   else
    __asm volatile("mrs %0, cntvct_el0" : "=r"(tsc));
#   endif
    return tsc;
    
#elif defined(__ARM_ARCH) && defined(__APPLE__)
    return mach_absolute_time();
    
#else
    struct timeval now;
    gettimeofday(&now, NULL);
    return (u64)now.tv_sec * 1000000 + now.tv_usec;
#endif
}


#ifdef __cplusplus
}
#endif

#endif
