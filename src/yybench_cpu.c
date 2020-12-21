/*==============================================================================
 * Copyright (C) 2020 YaoYuan <ibireme@gmail.com>.
 * Released under the MIT license (MIT).
 *============================================================================*/

#include "yybench_cpu.h"
#include "yybench_time.h"

#define REPEAT_2(x)   x x
#define REPEAT_4(x)   REPEAT_2(REPEAT_2(x))
#define REPEAT_8(x)   REPEAT_2(REPEAT_4(x))
#define REPEAT_16(x)  REPEAT_2(REPEAT_8(x))
#define REPEAT_32(x)  REPEAT_2(REPEAT_16(x))
#define REPEAT_64(x)  REPEAT_2(REPEAT_32(x))
#define REPEAT_128(x) REPEAT_2(REPEAT_64(x))
#define REPEAT_256(x) REPEAT_2(REPEAT_128(x))
#define REPEAT_512(x) REPEAT_2(REPEAT_256(x))


bool yy_cpu_setup_priority(void) {
#if defined(_WIN32)
    BOOL ret1 = SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
    BOOL ret2 = SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
    return ret1 && ret2;
#else
    int policy;
    struct sched_param param;
    pthread_t thread = pthread_self();
    pthread_getschedparam(thread, &policy, &param);
    param.sched_priority = sched_get_priority_max(policy);
    if (param.sched_priority != -1) {
        return pthread_setschedparam(pthread_self(), policy, &param) == 0;
    }
    return false;
#endif
}

void yy_cpu_spin(f64 second) {
    f64 end = yy_time_get_seconds() + second;
    while(yy_time_get_seconds() < end) {
        volatile int x = 0;
        while (x < 1000) x++;
    }
}

#if (yy_has_attribute(naked)) && YY_ARCH_ARM64
#define YY_CPU_RUN_INST_COUNT_A (8192 * (128 + 256))
#define YY_CPU_RUN_INST_COUNT_B (8192 * (512))

__attribute__((naked, noinline))
void yy_cpu_run_seq_a(void) {
    __asm volatile
    (
     "mov x0, #8192\n"
     "Loc_seq_loop_begin_a:\n"
     REPEAT_128("add x1, x1, x1\n")
     REPEAT_256("add x1, x1, x1\n")
     "subs x0, x0, #1\n"
     "bne Loc_seq_loop_begin_a\n"
     "ret\n"
     );
}

__attribute__((naked, noinline))
void yy_cpu_run_seq_b(void) {
    __asm volatile
    (
     "mov x0, #8192\n"
     "Loc_seq_loop_begin_b:\n"
     REPEAT_512("add x1, x1, x1\n")
     "subs x0, x0, #1\n"
     "bne Loc_seq_loop_begin_b\n"
     "ret\n"
     );
}

#elif (yy_has_attribute(naked)) && YY_ARCH_ARM32
#define YY_CPU_RUN_INST_COUNT_A (8192 * (128 + 256))
#define YY_CPU_RUN_INST_COUNT_B (8192 * (512))

__attribute__((naked, noinline))
void yy_cpu_run_seq_a(void) {
    __asm volatile
    (
     "mov.w r0, #8192\n"
     "Loc_seq_loop_begin_a:\n"
     REPEAT_128("add r1, r1, r1\n")
     REPEAT_256("add r1, r1, r1\n")
     "subs r0, r0, #1\n"
     "bne.w Loc_seq_loop_begin_a\n"
     "bx lr\n"
     );
}

__attribute__((naked, noinline))
void yy_cpu_run_seq_b(void) {
    __asm volatile
    (
     "mov.w r0, #8192\n"
     "Loc_seq_loop_begin_b:\n"
     REPEAT_512("add r1, r1, r1\n")
     "subs r0, r0, #1\n"
     "bne.w Loc_seq_loop_begin_b\n"
     "bx lr\n"
     );
}

#elif (yy_has_attribute(naked)) && (YY_ARCH_X64 || YY_ARCH_X86)
#define YY_CPU_RUN_INST_COUNT_A (8192 * (128 + 256))
#define YY_CPU_RUN_INST_COUNT_B (8192 * (512))

__attribute__((naked, noinline))
void yy_cpu_run_seq_a(void) {
    __asm volatile
    (
     "movl $8192, %eax\n"
     "seq_loop_begin_a:\n"
     REPEAT_128("addl %edx, %edx\n")
     REPEAT_256("addl %edx, %edx\n")
     "subl $1, %eax\n"
     "jne seq_loop_begin_a\n"
     "ret\n"
     );
}

__attribute__((naked, noinline))
void yy_cpu_run_seq_b(void) {
    __asm volatile
    (
     "movl $8192, %eax\n"
     "seq_loop_begin_b:\n"
     REPEAT_512("addl %edx, %edx\n")
     "subl $1, %eax\n"
     "jne seq_loop_begin_b\n"
     "ret\n"
     );
}

#else
#define YY_CPU_RUN_INST_COUNT_A (8192 * 4 * (32 + 64))
#define YY_CPU_RUN_INST_COUNT_B (8192 * 4 * (128))

/* These functions contains some `add` instructions with data dependence.
   This file should be compiled with optimization flag on.
   We hope that each line of the code in the inner loop may compiled as
   an `add` instruction, each `add` instruction takes 1 cycle, and inner kernel
   can fit in the L1i cache. Try: https://godbolt.org/z/d3GP1b */

u32 yy_cpu_run_seq_vals[8];

void yy_cpu_run_seq_a(void) {
    u32 loop = 8192;
    u32 v1 = yy_cpu_run_seq_vals[1];
    u32 v2 = yy_cpu_run_seq_vals[2];
    u32 v3 = yy_cpu_run_seq_vals[3];
    u32 v4 = yy_cpu_run_seq_vals[4];
    do {
        REPEAT_32( v1 += v4; v2 += v1; v3 += v2; v4 += v3; )
        REPEAT_64( v1 += v4; v2 += v1; v3 += v2; v4 += v3; )
    } while(--loop);
    yy_cpu_run_seq_vals[0] = v1;
}

void yy_cpu_run_seq_b(void) {
    u32 loop = 8192;
    u32 v1 = yy_cpu_run_seq_vals[1];
    u32 v2 = yy_cpu_run_seq_vals[2];
    u32 v3 = yy_cpu_run_seq_vals[3];
    u32 v4 = yy_cpu_run_seq_vals[4];
    do {
        REPEAT_128( v1 += v4; v2 += v1; v3 += v2; v4 += v3; )
    } while(--loop);
    yy_cpu_run_seq_vals[0] = v1;
}

#endif

static u64 yy_cycle_per_sec = 0;
static u64 yy_tick_per_sec = 0;

void yy_cpu_measure_freq(void) {
#define warmup_count 8
#define measure_count 128
    yy_time p1, p2;
    u64 ticks_a[measure_count];
    u64 ticks_b[measure_count];
    
    /* warm up CPU caches and stabilize the frequency */
    for (int i = 0; i < warmup_count; i++) {
        yy_cpu_run_seq_a();
        yy_cpu_run_seq_b();
        yy_time_get_current(&p1);
        yy_time_get_ticks();
    }
    
    /* run sequence a and b repeatedly, record ticks and times */
    yy_time_get_current(&p1);
    u64 t1 = yy_time_get_ticks();
    for (int i = 0; i < measure_count; i++) {
        u64 s1 = yy_time_get_ticks();
        yy_cpu_run_seq_a();
        u64 s2 = yy_time_get_ticks();
        yy_cpu_run_seq_b();
        u64 s3 = yy_time_get_ticks();
        ticks_a[i] = s2 - s1;
        ticks_b[i] = s3 - s2;
    }
    u64 t2 = yy_time_get_ticks();
    yy_time_get_current(&p2);
    
    /* calculate tick count per second, this value is high precision */
    f64 total_seconds = yy_time_to_seconds(&p2) - yy_time_to_seconds(&p1);
    u64 total_ticks = t2 - t1;
    yy_tick_per_sec = (u64)((f64)total_ticks / total_seconds);
    
    /* find the minimum ticks of each sequence to avoid inaccurate values
       caused by context switching, etc. */
    for (int i = 1; i < measure_count; i++) {
        if (ticks_a[i] < ticks_a[0]) ticks_a[0] = ticks_a[i];
        if (ticks_b[i] < ticks_b[0]) ticks_b[0] = ticks_b[i];
    }
    
    /* use the difference between two sequences to eliminate the overhead of
       loops and function calls */
    u64 one_ticks = ticks_b[0] - ticks_a[0];
    u64 one_insts = YY_CPU_RUN_INST_COUNT_B - YY_CPU_RUN_INST_COUNT_A;
    yy_cycle_per_sec = (u64)((f64)one_insts / (f64)one_ticks * (f64)yy_tick_per_sec);
#undef warmup_count
#undef measure_count
}

u64 yy_cpu_get_freq(void) {
    return yy_cycle_per_sec;
}

u64 yy_cpu_get_tick_per_sec(void) {
    return yy_tick_per_sec;
}

f64 yy_cpu_get_cycle_per_tick(void) {
    return (f64)yy_cycle_per_sec / (f64)yy_tick_per_sec;
}

f64 yy_cpu_tick_to_sec(u64 tick) {
    return tick / (f64)yy_tick_per_sec;;
}

u64 yy_cpu_tick_to_cycle(u64 tick) {
    return (u64)(tick * ((f64)yy_cycle_per_sec / (f64)yy_tick_per_sec));
}
