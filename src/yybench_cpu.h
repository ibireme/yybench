/*==============================================================================
 * Copyright (C) 2020 YaoYuan <ibireme@gmail.com>.
 * Released under the MIT license (MIT).
 *============================================================================*/

#ifndef yybench_cpu_h
#define yybench_cpu_h

#include "yybench_def.h"

#ifdef __cplusplus
extern "C" {
#endif


/*==============================================================================
 * CPU Utils
 *============================================================================*/

/** Try to increase the priority of the current thread.
    This method may used to reduce context switches of current thread. */
bool yy_cpu_setup_priority(void);

/** Let CPU 'spinning' for a while.
    This function may used to warm up CPU from power saving mode and
    stabilize the CPU frequency. */
void yy_cpu_spin(f64 second);

/** Measure current CPU frequency.
    This function may take about 1 second on 1GHz CPU.
    This function may returns inaccurate result in debug mode. */
void yy_cpu_measure_freq(void);

/** Returns CPU frequency in Hz.
    You should call yy_cpu_measure_freq() at least once before calling this
    function. */
u64 yy_cpu_get_freq(void);

/** Returns tick per second.
    You should call yy_cpu_measure_freq() at least once before calling this
    function. This function may used with yy_time_get_ticks() for benchmark. */
u64 yy_cpu_get_tick_per_sec(void);

/** Returns cpu cycles per tick.
    You should call yy_cpu_measure_freq() at least once before calling this
    function. This function may used with yy_time_get_ticks() for benchmark. */
f64 yy_cpu_get_cycle_per_tick(void);

/** Convert tick to second.
    You should call yy_cpu_measure_freq() at least once before calling this
    function. This function may used with yy_time_get_ticks() for benchmark. */
f64 yy_cpu_tick_to_sec(u64 tick);

/** Convert tick to CPU cycle.
    You should call yy_cpu_measure_freq() at least once before calling this
    function. This function may used with yy_time_get_ticks() for benchmark. */
u64 yy_cpu_tick_to_cycle(u64 tick);


#ifdef __cplusplus
}
#endif

#endif
