/*==============================================================================
 * Copyright (C) 2020 YaoYuan <ibireme@gmail.com>.
 * Released under the MIT license (MIT).
 *============================================================================*/

#ifndef yybench_perf_h
#define yybench_perf_h

#include "yybench_def.h"

#ifdef __cplusplus
extern "C" {
#endif


/*==============================================================================
 Performance Monitor Counter
 
 Currently only support Linux, macOS and iOS with root privileges.
 Do not use it with Intel VTunes, Linux perf, Xcode Instruments Counters,
 or it may produce wrong results.
 
 Notice: A CPU has only a limited number configurable counters. Linux's perf
 subsystem use multiplexing to allow us to monitor multiple events, but may
 add more overhead. Apple XNU's KPC/kperf module doesn't use multiplexing,
 so we can only monitor a limited number of event at the same time.
 
 Usage:
 
     // load perf module
     bool loaded = yy_perf_load(true);
     if (!loaded) return;
     
     // create perf config, add event
     yy_perf *perf = yy_perf_new();
     yy_perf_add_event(perf, YY_PERF_EVENT_CYCLES);
     yy_perf_add_event(perf, YY_PERF_EVENT_INSTRUCTIONS);
     yy_perf_add_event(perf, YY_PERF_EVENT_BRANCHES);
     yy_perf_add_event(perf, YY_PERF_EVENT_BRANCH_MISSES);
     
     if (!yy_perf_open(perf)) return;
     
     yy_perf_start_counting(perf);
     // code to profile...
     yy_perf_stop_counting(perf);
     
     // get results
     u32 ev_count = yy_perf_get_event_count(perf);
     const char **names = yy_perf_get_event_names(perf);
     u64 *vals = yy_perf_get_counters(perf);
     
     // print results
     for (u32 i = 0; i < ev_count; i++) {
        printf("%s: %llu\n",names[i], vals[i]);
     }
     
     // close and free resource
     yy_perf_close(perf);
     yy_perf_free(perf);
 
 
 *============================================================================*/

#define YY_PERF_AVAILABLE           0
#define YY_PERF_AVAILABLE_WINDOWS   0
#define YY_PERF_AVAILABLE_LINUX     0
#define YY_PERF_AVAILABLE_APPLE     0


/** Common PMU event */
typedef enum yy_perf_event {
    /* Not a real event, used as placeholder */
    YY_PERF_EVENT_NONE = 0,
    
    /* CPU cycles count */
    YY_PERF_EVENT_CYCLES,
    
    /* All retired instruction count */
    YY_PERF_EVENT_INSTRUCTIONS,
    
    /* Branch instruction count */
    YY_PERF_EVENT_BRANCHES,
    
    /* Branch instruction mispredict count */
    YY_PERF_EVENT_BRANCH_MISSES,
    
    /* Level 1 instruction cache load count */
    YY_PERF_EVENT_L1I_LOADS,
    
    /* Level 1 instruction cache load miss count */
    YY_PERF_EVENT_L1I_LOAD_MISSES,
    
    /* Level 1 data cache load count */
    YY_PERF_EVENT_L1D_LOADS,
    
    /* Level 1 data cache load miss count */
    YY_PERF_EVENT_L1D_LOAD_MISSES,
    
    /* Level 1 data cache store count */
    YY_PERF_EVENT_L1D_STORES,
    
    /* Level 1 data cache store miss count */
    YY_PERF_EVENT_L1D_STORE_MISSES,
    
    /* Last-Level Cache load count */
    YY_PERF_EVENT_LLC_LOADS,
    
    /* Last-Level Cache load miss count */
    YY_PERF_EVENT_LLC_LOAD_MISSES,
    
    /* Last-Level Cache store count */
    YY_PERF_EVENT_LLC_STORES,
    
    /* Last-Level Cache store miss count */
    YY_PERF_EVENT_LLC_STORE_MISSES,
} yy_perf_event;


/** A perf object. */
typedef struct yy_perf yy_perf;

/**
 Load perf module.
 This function should be called once before any other functions in this file..
 @param print_message_on_error true to print error message on error.
 @return true on success.
 */
bool yy_perf_load(bool print_message_on_error);

/** Creates a perf object. */
yy_perf *yy_perf_new(void);

/** Stop and free the perf object. */
void yy_perf_free(yy_perf *perf);

/** Add a common event. */
bool yy_perf_add_event(yy_perf *perf, yy_perf_event event);

/** Remove all events. */
bool yy_perf_remove_all_events(yy_perf *perf);

/** Whether an event is availabie for current host. */
bool yy_perf_event_available(yy_perf *perf, yy_perf_event ev);

/** Add an event by name, available for macOS/iOS.
    Return false if the event is not supported.
    
    @param ev_name
    You can get event names in file /usr/share/kpep/xxx.plist.
    For example: "INST_RETIRED.ANY".
 
    @param ev_alias
    You can give an alias name for this event, or NULL if you don't need it.
 */
bool yy_perf_add_event_apple(yy_perf *perf, const char *ev_name, const char *ev_alias);

/** Whether an event is availabie for current host. */
bool yy_perf_event_available_apple(yy_perf *perf, const char *ev_name);

/** Add an event by type, available for Linux.
    Return false if the event is not supported.
    
    @param ev_value
    You can get common names in file <linux/perf_event.h>, or "man perf_event_open".
    For example PERF_EVENT_MAKE(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES).
    
    You can also use some raw values to build a event.
    For example, you can find some raw values in Android project:
    https://android.googlesource.com/platform/system/extras/+/master/simpleperf/event_type_table.h
    and make event value with: PERF_EVENT_MAKE(PERF_TYPE_RAW, 0x3)
    
    @param ev_alias
    You can give an alias name for this event, or NULL if you don't need it.
    */
bool yy_perf_add_event_linux(yy_perf *perf, u64 ev_value, const char *ev_alias);

/** Whether an event is availabie for current host. */
bool yy_perf_event_available_linux(yy_perf *perf, u64 ev_value);

/** Get current event count. */
u32 yy_perf_get_event_count(yy_perf *perf);

/** Get current event names. */
const char **yy_perf_get_event_names(yy_perf *perf);

/** Open perf, apply the config to kernel.
    After open, you can not add or remove events. */
bool yy_perf_open(yy_perf *perf);

/** Close perf (and close counting). */
bool yy_perf_close(yy_perf *perf);

/** Whether perf is opened. */
bool yy_perf_is_opened(yy_perf *perf);

/** Start perf counting. */
bool yy_perf_start_counting(yy_perf *perf);

/** Stop perf counting. */
bool yy_perf_stop_counting(yy_perf *perf);

/** Whether perf is counting. */
bool yy_perf_is_counting(yy_perf *perf);

/** Get counter values. */
u64 *yy_perf_get_counters(yy_perf *perf);


/*==============================================================================
 * Linux
 
 * Header:
 * /usr/include/linux/perf_event.h
 *
 * Doc
 * man perf_event_open
 *============================================================================*/

#ifndef YY_LINUX_HAS_PERF
#   ifdef __linux__
#       if yy_has_include(<asm/unistd.h>) && \
          (yy_has_include(<linux/perf_event.h>) || \
           yy_has_include("linux/perf_event.h"))
#           define YY_LINUX_HAS_PERF 1
#       endif
#   endif
#endif

#if YY_LINUX_HAS_PERF
#undef  YY_PERF_AVAILABLE
#define YY_PERF_AVAILABLE           1
#undef  YY_PERF_AVAILABLE_LINUX
#define YY_PERF_AVAILABLE_LINUX     1

#include <asm/unistd.h>
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <unistd.h>

/*
 Given a list of parameters, returns a file descriptor,
 for use in subsequent system calls (ioctl(), read(), close(), etc.).

 @param pid Specifying which process to monitor:
            pid = -1: all processes/threads
            pid = 0: calling process/thread
            pid > 0: the specified process/thread
 @param cpu Specifying which CPU to monitor:
            cpu = -1: any CPU
            cpu >= 0: the specified CPU
 @param group_fd Allows event groups to be created.
 @param flags See `man perf_event_open` for all available values.
 @warning (pid > 0) requires ptrace permission.
          (pid = -1 and cpu = -1) is invalid and will return an error.
          (pid = -1 and cpu >= 0) requires CAP_SYS_ADMIN capability.

 @see man perf_event_open
 */
static int yy_perf_event_open(struct perf_event_attr *attr, pid_t pid, int cpu,
                              int group_fd, unsigned long flags) {
    return (int)syscall(__NR_perf_event_open, attr, pid, cpu, group_fd, flags);
}

#define PERF_EVENT_MAKE(type, config) (((u64)type << 32) | ((u32)config))

#define PERF_EVENT_MAKE_CACHE(id, op, result) \
        PERF_EVENT_MAKE(PERF_TYPE_HW_CACHE, ((u32)id | ((u32)op << 8) | ((u32)result << 16)))

#define PERF_EVENT_GET_TYPE(ev)  ((u32)(ev >> 32))

#define PERF_EVENT_GET_CONFIG(ev)  ((u32)(ev))

#endif /* LINUX */


/*==============================================================================
 * Apple
 *============================================================================*/

#ifndef YY_APPLE_HAS_PERF
#   if defined(__APPLE__)
#       define YY_APPLE_HAS_PERF 1
#   endif
#endif

#if YY_APPLE_HAS_PERF
#undef  YY_PERF_AVAILABLE
#define YY_PERF_AVAILABLE       1
#undef  YY_PERF_AVAILABLE_APPLE
#define YY_PERF_AVAILABLE_APPLE 1


/*==============================================================================
 * Apple
 *
 * Frameworks:
 * /System/Library/PrivateFrameworks/kperf.framework
 * /System/Library/PrivateFrameworks/kperfdata.framework
 * /Applications/Xcode.app/Contents/SharedFrameworks/DVTInstrumentsFoundation.framework
 *
 * CPU database path:
 * /usr/share/kpep/
 *
 * Kernel source: https://opensource.apple.com/tarballs/xnu/
 * /xnu/bsd/kern/kern_kpc.c
 * /xnu/osfmk/kern/kpc.h
 *============================================================================*/


/**
 Load kperf and kperfdata dynamic library.
 Return true if success.
 */
bool kperf_load(bool print_message_on_error);

/*==============================================================================
 * Apple
 * kperf.framework
 * Most functions requires root privileges, or process is "blessed".
 *============================================================================*/

/* cross-platform class constants */
#define KPC_CLASS_FIXED         (0)
#define KPC_CLASS_CONFIGURABLE  (1)
#define KPC_CLASS_POWER         (2)
#define KPC_CLASS_RAWPMU        (3)

/* cross-platform class mask constants, use these masks as `classes` param */
#define KPC_CLASS_FIXED_MASK         (1u << KPC_CLASS_FIXED)        /* 1 */
#define KPC_CLASS_CONFIGURABLE_MASK  (1u << KPC_CLASS_CONFIGURABLE) /* 2 */
#define KPC_CLASS_POWER_MASK         (1u << KPC_CLASS_POWER)        /* 4 */
#define KPC_CLASS_RAWPMU_MASK        (1u << KPC_CLASS_RAWPMU)       /* 8 */

/* PMU version constants */
#define KPC_PMU_ERROR     (0)
#define KPC_PMU_INTEL_V3  (1)
#define KPC_PMU_ARM_APPLE (2)
#define KPC_PMU_INTEL_V2  (3)
#define KPC_PMU_ARM_V2    (4)

/* Bits to get CPU counters for all CPUs. */
#define KPC_ALL_CPUS (1u << 31)


/// Prints CPU string to the buffer (same as snprintf), such as "cpu_7_8_10b282dc_46".
/// @return string's length, or negative value if error occurs.
/// @note This method does not requires root privileges.
/// @name get(hw.cputype), get(hw.cpusubtype), get(hw.cpufamily), get(machdep.cpu.model)
extern int (*kpc_cpu_string)(char *buf, usize buf_size);

/// Returns PMU (Performance Monitoring Unit) version for local machine.
/// @return `KPC_PMU_ERROR` if error occurs.
/// @name get(kpc.pmu_version)
extern u32 (*kpc_pmu_version)(void);


/// Returns running PMC classes.
/// @return 0 if error occurs or no class is set.
/// @name get(kpc.counting)
extern u32 (*kpc_get_counting)(void);

/// Sets PMC classes to enable counting, or set 0 to shutdown counting.
/// @return 0 for success.
/// @name set(kpc.counting)
extern int (*kpc_set_counting)(u32 classes);

/// Returns running PMC classes for current thread.
/// @return 0 if error occurs or no class is set.
/// @name get(kpc.thread_counting)
extern u32 (*kpc_get_thread_counting)(void);

/// Sets PMC classes to enable counting for current thread, or set 0 to shutdown counting.
/// @return 0 for success.
/// @name set(kpc.thread_counting)
extern int (*kpc_set_thread_counting)(u32 classes);


/// Returns how many config registers there are for a given config.
/// For example: Intel may returns 1 for `KPC_CLASS_FIXED_MASK`,
///                        returns 4 for `KPC_CLASS_CONFIGURABLE_MASK`.
/// @note This method does not requires root privileges.
/// @name get(kpc.config_count)
extern u32 (*kpc_get_config_count)(u32 classes);

/// Get counter configs.
/// @param buf_in Config buffer, should not smaller than kpc_get_config_count()
/// @return 0 for success.
/// @name get(kpc.config_count), get(kpc.config)
extern int (*kpc_get_config)(u32 classes, u64 *buf_out);

/// Set counter configs.
/// @param buf_in Config buffer, should not smaller than kpc_get_config_count()
/// @return 0 for success.
/// @name get(kpc.config_count), set(kpc.config)
extern int (*kpc_set_config)(u32 classes, u64 *buf_in);


/// Returns how many counters there are for a given config.
/// For example: Intel may returns 3 for `KPC_CLASS_FIXED_MASK`,
///                        returns 4 for `KPC_CLASS_CONFIGURABLE_MASK`.
/// @note This method does not requires root privileges.
/// @name get(kpc.counter_count)
extern u32 (*kpc_get_counter_count)(u32 classes);

/// Get counter's value.
/// If `all_cpus` is true, the buffer count should not smaller than (cpu_count * counter_count).
/// otherwize, the buffer count should not smaller than (counter_count).
/// @see kpc_get_counter_count(), kpc_cpu_count().
/// @param all_cpus true for all CPUs, false for current cpu.
/// @param classes The class masks.
/// @param curcpu Output current cpu id, pass NULL if you do not need it.
/// @param buf Buffer to receive counter's value.
/// @return 0 for success.
/// @name get(hw.ncpu), get(kpc.counter_count), get(kpc.counters)
extern int (*kpc_get_cpu_counters)(bool all_cpus, u32 classes, int *curcpu, u64 *buf);

/// Get counter's value for current thread accumulation.
/// @param tid Thread id, should be 0.
/// @param buf_count The buf's count (not bytes), should not smaller than kpc_get_counter_count().
/// @param buf Buffer to receive counter's value.
/// @return 0 for success.
/// @name get(kpc.thread_counters)
extern int (*kpc_get_thread_counters)(u32 tid, u32 buf_count, u64 *buf);


/// acquire/release the counters used by the Power Manager.
/// @param val 1:acquire, 0:release
/// @return 0 for success.
/// @name set(kpc.force_all_ctrs)
extern int (*kpc_force_all_ctrs_set)(int val);

/// Get the state of all_ctrs.
/// @return 0 for success.
/// @name get(kpc.force_all_ctrs)
extern int (*kpc_force_all_ctrs_get)(int *val_out);

/// Reset kperf: stop sampling, kdebug, timers and actions.
/// @return 0 for success.
extern int (*kperf_reset)(void);


/*==============================================================================
 * kperfdata.framework
 *============================================================================*/

#define KPEP_ARCH_I386      0
#define KPEP_ARCH_X86_64    1
#define KPEP_ARCH_ARM       2
#define KPEP_ARCH_ARM64     3

/// kpep event (size: 48/28 bytes on 64/32 bit OS)
typedef struct kpep_event {
    const char *name;           ///< unique name of a event, such as "INST_RETIRED.ANY"
    const char *description;    ///< description for this event.
    const char *errata;         ///< usually NULL
    const char *alias;          ///< alias name, such as "Instructions", "Cycles"
    const char *fallback;       ///< fallback event name for fixed counter
    u32 mask;
    u8 number;
    u8 umask;
    u8 reserved;
    u8 is_fixed;
} kpep_event;

/// kpep database (size: 144/80 bytes on 64/32 bit OS)
typedef struct kpep_db {
    const char *name;   ///< database name, such as "haswell"
    const char *cpu_id; ///< plist name, such as "cpu_7_8_10b282dc"
    const char *marketing_name;///< marketing name, such as "Intel Haswell"
    void *plist_data; ///< plist data (CFDataRef), usually NULL
    void *event_dict; ///< all event dictionary (CFDictionaryRef <event_name (CFSTR), kpep_event *>)
    kpep_event *events_arr; ///< event struct buffer (sizeof(kpep_event) * events_count)
    kpep_event **fixed_events; ///< fixed counter events (sizeof(kpep_event *) * fixed_counter_count)
    void *aliases_dic;///< aliases dictionary (CFDictionaryRef <event_name (CFSTR), kpep_event *>)
    usize reserved_1;
    usize reserved_2;
    usize reserved_3;
    usize events_count; ///< all events count
    usize aliases_count;
    usize fixed_counter_count;
    usize config_counter_count;
    usize power_counter_count;
    u32 archtecture; ///< CPU arch, such as `KPEP_ARCH_X86_64`
    u32 fixed_counter_bits;
    u32 config_counter_bits;
    u32 power_counter_bits;
} kpep_db;

/// kpep config (size: 80/44 bytes on 64/32 bit OS)
typedef struct kpep_config {
    kpep_db *db;
    kpep_event **ev_arr; ///< (sizeof(kpep_event *) * counter_count), init NULL
    usize *ev_map; ///< (sizeof(usize *) * counter_count), init 0
    usize *ev_idx; ///< (sizeof(usize *) * counter_count), init -1
    u32 *flags; ///< (sizeof(u32 *) * counter_count), init 0
    u64 *kpc_periods; ///< (sizeof(u64 *) * counter_count), init 0
    usize events_count; /// kpep_config_events_count()
    usize counters_count;
    u32 kpc_classes;
    u32 config_counter;
    u32 power_counter;
    u32 reserved;
} kpep_config;


/// Create a config.
/// @return 0 for success.
extern int (*kpep_config_create)(kpep_db *db, kpep_config **cfg_ptr);

/// Free the config.
extern void (*kpep_config_free)(kpep_config *cfg);

/// Add an event to config.
/// @param cfg The config.
/// @param ev_ptr One event.
/// @param flag Pass 1.
/// @param err Err flag, can be NULL.
/// @return 0: success
///         1: error trying to add event
///         12: conflicts with previously added events
///         13: requires power counters that are not enabled
///         14: could not locate the event
///         other: unexpected error
extern int (*kpep_config_add_event)(kpep_config *cfg, kpep_event **ev_ptr, u32 flag, u32 *err);

/// Remove event at index.
/// @return 0 for success.
extern int (*kpep_config_remove_event)(kpep_config *cfg, usize idx);

/// Force all counters.
/// @return 0 for success.
extern int (*kpep_config_force_counters)(kpep_config *cfg);

/// Get events count.
/// @return 0 for success.
extern int (*kpep_config_events_count)(kpep_config *cfg, usize *count_ptr);

/// Get all event pointers.
/// @param buf A buffer to receive event pointers.
/// @param buf_size The buffer's size in bytes.
/// @return 0 for success.
extern int (*kpep_config_events)(kpep_config *cfg, kpep_event **buf, usize buf_size);

/// Get kpc register config.
/// @param buf A buffer to receive kpc register configs.
/// @param buf_size The buffer's size in bytes.
/// @return 0 for success.
extern int (*kpep_config_kpc)(kpep_config *cfg, u64 *buf, usize buf_size);

/// Get kpc register config count.
/// @return 0 for success.
extern int (*kpep_config_kpc_count)(kpep_config *cfg, usize *count_ptr);

/// Get kpc classes.
/// @return 0 for success.
extern int (*kpep_config_kpc_classes)(kpep_config *cfg, u32 *classes_ptr);

/// Apply config to CPU.
/// @return 0 for success.
/// @see kpep_config_kpc(), kpc_force_all_ctrs_set(1), kpc_set_config()
extern int (*kpep_config_apply)(kpep_config *cfg);


/// Create a kpep database.
/// The database file is in "/usr/share/kpep"
/// @param cpu_name CPU name, for example "A12", "haswell".
///                 Pass NULL for current CPU.
/// @return 0 for success.
extern int (*kpep_db_create)(const char *cpu_name, kpep_db **db_ptr);

/// Free the kpep database.
extern void (*kpep_db_free)(kpep_db *db);

/// Get the database's name.
/// @return 0 for success.
extern int (*kpep_db_name)(kpep_db *db, const char **name);

/// Get the database's arch.
/// @return 0 for success.
extern int (*kpep_db_architecture)(kpep_db *db, u32 *arch);

/// Get the event alias count.
/// @return 0 for success.
extern int (*kpep_db_aliases_count)(kpep_db *db, usize *count);

/// Get all alias.
/// @param buf A buffer to receive all alias strings.
/// @param buf_size The buffer's size in bytes.
/// @return 0 for success.
extern int (*kpep_db_aliases)(kpep_db *db, const char **buf, usize buf_size);

/// Get counters count for given classes.
/// @return 0 for success.
extern int (*kpep_db_counters_count)(kpep_db *db, u8 classes, usize *count);

/// Get all event count.
/// @return 0 for success.
extern int (*kpep_db_events_count)(kpep_db *db, usize *count);

/// Get all events.
/// @param buf A buffer to receive all event pointers.
/// @param buf_size The buffer's size in bytes.
/// @return 0 for success.
extern int (*kpep_db_events)(kpep_db *db, kpep_event **buf, usize buf_size);

/// Get one event by name.
/// @return 0 for success.
extern int (*kpep_db_event)(kpep_db *db, const char *name, kpep_event **ev_ptr);


/// Get event's name.
/// @return 0 for success.
extern int (*kpep_event_name)(kpep_event *ev, const char **name_ptr);

/// Get event's alias.
/// @return 0 for success.
extern int (*kpep_event_alias)(kpep_event *ev, const char **alias_ptr);

/// Get event's description.
/// @return 0 for success.
extern int (*kpep_event_description)(kpep_event *ev, const char **str_ptr);

/// Get event's errata.
/// @return 0 for success.
extern int (*kpep_event_errata)(kpep_event *ev, const char **errata_ptr);


#endif /* APPLE */


#ifdef __cplusplus
}
#endif

#endif
