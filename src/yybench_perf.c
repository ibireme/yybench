/*==============================================================================
 * Copyright (C) 2020 YaoYuan <ibireme@gmail.com>.
 * Released under the MIT license (MIT).
 *============================================================================*/

#include "yybench_perf.h"
#include <stdio.h>


/*==============================================================================
 * Linux
 *============================================================================*/

#if YY_LINUX_HAS_PERF

struct yy_perf {
    u32 count;
    u32 capacity;
    u64 *events;
    const char **names;
    u64 *buffer;
    u64 *ids;
    u64 *counters;
    int fd;
    bool is_opened;
    bool is_counting;
};

static const char *perf_event_get_name(u64 event) {
    // see <linux/perf_event.h>
    u32 type = PERF_EVENT_GET_TYPE(event);
    u32 cfg = PERF_EVENT_GET_CONFIG(event);
    switch (type) {
        case PERF_TYPE_HARDWARE:
            switch (cfg) {
                case 0: return "cpu-cycles";
                case 1: return "instructions";
                case 2: return "cache-references";
                case 3: return "cache-misses";
                case 4: return "branches";
                case 5: return "branch-misses";
                case 6: return "bus-cycles";
                case 7: return "stalled-cycles-frontend";
                case 8: return "stalled-cycles-backend";
                case 9: return "ref-cpu-cycles";
                default: return "unknown-hardware-event";
            }
        case PERF_TYPE_SOFTWARE:
            switch (cfg) {
                case 0: return "cpu-clock";
                case 1: return "task-clock";
                case 2: return "page-faults";
                case 3: return "context-switches";
                case 4: return "cpu-migrations";
                case 5: return "page-faults-min";
                case 6: return "page-faults-maj";
                case 7: return "alignment-faults";
                case 8: return "emulation-faults";
                case 9: return "dummy";
                case 10: return "bpf-output";
                default: return "unknown-software-event";
            }
        case PERF_TYPE_HW_CACHE:
            switch (cfg) {
                case (0 | (0 << 8) | (0 << 16)): return "L1d-read";
                case (0 | (0 << 8) | (1 << 16)): return "L1d-read-misses";
                case (0 | (1 << 8) | (0 << 16)): return "L1d-write";
                case (0 | (1 << 8) | (1 << 16)): return "L1d-write-misses";
                case (0 | (2 << 8) | (0 << 16)): return "L1d-prefetch";
                case (0 | (2 << 8) | (1 << 16)): return "L1d-prefetch-misses";

                case (1 | (0 << 8) | (0 << 16)): return "L1i-read";
                case (1 | (0 << 8) | (1 << 16)): return "L1i-read-misses";
                case (1 | (1 << 8) | (0 << 16)): return "L1i-write";
                case (1 | (1 << 8) | (1 << 16)): return "L1i-write-misses";
                case (1 | (2 << 8) | (0 << 16)): return "L1i-prefetch";
                case (1 | (2 << 8) | (1 << 16)): return "L1i-prefetch-misses";

                case (2 | (0 << 8) | (0 << 16)): return "LLC-read";
                case (2 | (0 << 8) | (1 << 16)): return "LLC-read-misses";
                case (2 | (1 << 8) | (0 << 16)): return "LLC-write";
                case (2 | (1 << 8) | (1 << 16)): return "LLC-write-misses";
                case (2 | (2 << 8) | (0 << 16)): return "LLC-prefetch";
                case (2 | (2 << 8) | (1 << 16)): return "LLC-prefetch-misses";

                case (3 | (0 << 8) | (0 << 16)): return "TDLB-read";
                case (3 | (0 << 8) | (1 << 16)): return "TDLB-read-misses";
                case (3 | (1 << 8) | (0 << 16)): return "TDLB-write";
                case (3 | (1 << 8) | (1 << 16)): return "TDLB-write-misses";
                case (3 | (2 << 8) | (0 << 16)): return "TDLB-prefetch";
                case (3 | (2 << 8) | (1 << 16)): return "TDLB-prefetch-misses";

                case (4 | (0 << 8) | (0 << 16)): return "ITLB-read";
                case (4 | (0 << 8) | (1 << 16)): return "ITLB-read-misses";
                case (4 | (1 << 8) | (0 << 16)): return "ITLB-write";
                case (4 | (1 << 8) | (1 << 16)): return "ITLB-write-misses";
                case (4 | (2 << 8) | (0 << 16)): return "ITLB-prefetch";
                case (4 | (2 << 8) | (1 << 16)): return "ITLB-prefetch-misses";

                case (5 | (0 << 8) | (0 << 16)): return "BPU-read";
                case (5 | (0 << 8) | (1 << 16)): return "BPU-read-misses";
                case (5 | (1 << 8) | (0 << 16)): return "BPU-write";
                case (5 | (1 << 8) | (1 << 16)): return "BPU-write-misses";
                case (5 | (2 << 8) | (0 << 16)): return "BPU-prefetch";
                case (5 | (2 << 8) | (1 << 16)): return "BPU-prefetch-misses";

                case (6 | (0 << 8) | (0 << 16)): return "node-read";
                case (6 | (0 << 8) | (1 << 16)): return "node-read-misses";
                case (6 | (1 << 8) | (0 << 16)): return "node-write";
                case (6 | (1 << 8) | (1 << 16)): return "node-write-misses";
                case (6 | (2 << 8) | (0 << 16)): return "node-prefetch";
                case (6 | (2 << 8) | (1 << 16)): return "node-prefetch-misses";
                default: return "unknown-cache-event";
            }
        case PERF_TYPE_RAW:
            return "raw";
        default:
            return "unknown";
    }
}

static u64 perf_event_conv(yy_perf_event ev) {
    switch (ev) {
        case YY_PERF_EVENT_CYCLES:
            return PERF_EVENT_MAKE(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES);
        case YY_PERF_EVENT_INSTRUCTIONS:
            return PERF_EVENT_MAKE(PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS);
        case YY_PERF_EVENT_BRANCHES:
            return PERF_EVENT_MAKE(PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_INSTRUCTIONS);
        case YY_PERF_EVENT_BRANCH_MISSES:
            return PERF_EVENT_MAKE(PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_MISSES);
        case YY_PERF_EVENT_L1I_LOADS:
            return PERF_EVENT_MAKE_CACHE(PERF_COUNT_HW_CACHE_L1I, PERF_COUNT_HW_CACHE_OP_READ, PERF_COUNT_HW_CACHE_RESULT_ACCESS);
        case YY_PERF_EVENT_L1I_LOAD_MISSES:
            return PERF_EVENT_MAKE_CACHE(PERF_COUNT_HW_CACHE_L1I, PERF_COUNT_HW_CACHE_OP_READ, PERF_COUNT_HW_CACHE_RESULT_MISS);
        case YY_PERF_EVENT_L1D_LOADS:
            return PERF_EVENT_MAKE_CACHE(PERF_COUNT_HW_CACHE_L1D, PERF_COUNT_HW_CACHE_OP_READ, PERF_COUNT_HW_CACHE_RESULT_ACCESS);
        case YY_PERF_EVENT_L1D_LOAD_MISSES:
            return PERF_EVENT_MAKE_CACHE(PERF_COUNT_HW_CACHE_L1D, PERF_COUNT_HW_CACHE_OP_READ, PERF_COUNT_HW_CACHE_RESULT_MISS);
        case YY_PERF_EVENT_L1D_STORES:
            return PERF_EVENT_MAKE_CACHE(PERF_COUNT_HW_CACHE_L1D, PERF_COUNT_HW_CACHE_OP_WRITE, PERF_COUNT_HW_CACHE_RESULT_ACCESS);
        case YY_PERF_EVENT_L1D_STORE_MISSES:
            return PERF_EVENT_MAKE_CACHE(PERF_COUNT_HW_CACHE_L1D, PERF_COUNT_HW_CACHE_OP_WRITE, PERF_COUNT_HW_CACHE_RESULT_MISS);
        case YY_PERF_EVENT_LLC_LOADS:
            return PERF_EVENT_MAKE_CACHE(PERF_COUNT_HW_CACHE_LL, PERF_COUNT_HW_CACHE_OP_READ, PERF_COUNT_HW_CACHE_RESULT_ACCESS);
        case YY_PERF_EVENT_LLC_LOAD_MISSES:
            return PERF_EVENT_MAKE_CACHE(PERF_COUNT_HW_CACHE_LL, PERF_COUNT_HW_CACHE_OP_READ, PERF_COUNT_HW_CACHE_RESULT_MISS);
        case YY_PERF_EVENT_LLC_STORES:
            return PERF_EVENT_MAKE_CACHE(PERF_COUNT_HW_CACHE_LL, PERF_COUNT_HW_CACHE_OP_WRITE, PERF_COUNT_HW_CACHE_RESULT_ACCESS);
        case YY_PERF_EVENT_LLC_STORE_MISSES:
            return PERF_EVENT_MAKE_CACHE(PERF_COUNT_HW_CACHE_LL, PERF_COUNT_HW_CACHE_OP_WRITE, PERF_COUNT_HW_CACHE_RESULT_MISS);
        default: return 0;
    }
}

static bool perf_capacity_grow(yy_perf *perf, u32 capacity) {
    u64 *events = NULL;
    const char **names = NULL;
    u64 *buffer = NULL;
    u64 *ids = NULL;
    u64 *counters = NULL;

    events = calloc(capacity, sizeof(u64));
    if (!events) goto fail;
    names = calloc(capacity, sizeof(char *));
    if (!names) goto fail;
    buffer = calloc((capacity + 1) * 2, sizeof(u64));
    if (!buffer) goto fail;
    ids = calloc(capacity, sizeof(u64));
    if (!ids) goto fail;
    counters = calloc(capacity, sizeof(u64));
    if (!counters) goto fail;
    if (perf->count) {
        memcpy(events, perf->events, perf->count * sizeof(u64));
        memcpy(names, perf->names, perf->count * sizeof(char *));
        memcpy(buffer, perf->buffer, (perf->count + 1) * 2 * sizeof(u64));
        memcpy(counters, perf->counters, perf->count * sizeof(u64));
        free(perf->events);
        free(perf->names);
        free(perf->buffer);
        free(perf->ids);
        free(perf->counters);
    }
    perf->events = events;
    perf->names = names;
    perf->buffer = buffer;
    perf->ids = ids;
    perf->counters = counters;
    perf->capacity = capacity;
    return true;

fail:
    if (events) free(events);
    if (names) free(names);
    if (buffer) free(buffer);
    if (ids) free(ids);
    if (counters) free(counters);
}

bool perf_open_test(u64 ev) {
    struct perf_event_attr pe = {0};
    pe.type = PERF_EVENT_GET_TYPE(ev);
    pe.config = PERF_EVENT_GET_CONFIG(ev);
    pe.size = sizeof(struct perf_event_attr);
    pe.disabled = 1;
    int fd = yy_perf_event_open(&pe, 0, -1, -1, 0);
    if (fd == -1) {
        return false;
    }
    close(fd);
    return true;
}

bool yy_perf_load(bool print_message_on_error) {
    static bool loaded = false;
    if (loaded) return true;

    if (!perf_open_test(PERF_EVENT_MAKE(PERF_TYPE_HARDWARE, YY_PERF_EVENT_CYCLES))) {
        if (print_message_on_error) {
            fprintf(stderr, "Cannot open perf event, this requires root privileges.\n");
            fprintf(stderr, "If it runs on a VM, virtual CPU performance counters should be enabled.\n");
        }
        return false;
    }

    loaded = true;
    return true;
}

yy_perf *yy_perf_new(void) {
    yy_perf *perf = (yy_perf *)calloc(1, sizeof(yy_perf));
    if (!perf) return NULL;
    if (!perf_capacity_grow(perf, 8)) return false;
    return perf;
}

void yy_perf_free(yy_perf *perf) {
    if (!perf) return;
    yy_perf_close(perf);
    if (perf->events) free(perf->events);
    if (perf->names) free(perf->names);
    if (perf->buffer) free(perf->buffer);
    if (perf->ids) free(perf->ids);
    if (perf->counters) free(perf->counters);
    memset(perf, 0, sizeof(yy_perf));
    free(perf);
}

bool yy_perf_add_event(yy_perf *perf, yy_perf_event event) {
    u64 ev = perf_event_conv(event);
    return yy_perf_add_event_linux(perf, ev, NULL);
}

bool yy_perf_remove_all_events(yy_perf *perf) {
    if (!perf) return false;
    if (perf->is_opened || perf->is_counting) return false;
    perf->count = 0;
    return true;
}

bool yy_perf_event_available(yy_perf *perf, yy_perf_event event) {
    u64 ev = perf_event_conv(event);
    return yy_perf_event_available_linux(perf, ev);
}

bool yy_perf_add_event_apple(yy_perf *perf, const char *ev_name, const char *ev_alias) {
    return false;
}

bool yy_perf_event_available_apple(yy_perf *perf, const char *ev_name) {
    return false;
}

bool yy_perf_add_event_linux(yy_perf *perf, u64 ev_value, const char *ev_alias) {
    if (!perf) return false;
    if (perf->is_opened || perf->is_counting) return false;
    if (!yy_perf_event_available_linux(perf, ev_value)) return false;

    if (perf->count >= perf->capacity) {
        if (!perf_capacity_grow(perf, perf->capacity * 2)) {
            return false;
        }
    }

    perf->events[perf->count] = ev_value;
    perf->names[perf->count] = ev_alias ? ev_alias : perf_event_get_name(ev_value);
    perf->count++;
    return true;
}

bool yy_perf_event_available_linux(yy_perf *perf, u64 ev_value) {
    return perf_open_test(ev_value);
}

u32 yy_perf_get_event_count(yy_perf *perf) {
    if (perf) return perf->count;
    return 0;
}

const char **yy_perf_get_event_names(yy_perf *perf) {
    if (!perf || !perf->count) return NULL;
    return perf->names;
}

bool yy_perf_open(yy_perf *perf) {
    if (!perf) return false;
    if (perf->count == 0) return false;
    if (perf->is_opened) return true;

    struct perf_event_attr pe = {0};
    int ret = 0;
    int fd = 0;
    pid_t pid = 0; // calling process
    int cpu = -1; // any CPU
    int group = -1; // no group
    unsigned long long flags = 0; // no flag

    pe.size = sizeof(struct perf_event_attr);
    pe.disabled = 1;
    pe.exclude_kernel = 1;
    pe.exclude_hv = 1;
    pe.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID;

    perf->fd = -1;
    for (u32 i = 0; i < perf->count; i++) {
        u64 ev = perf->events[i];
        pe.type = PERF_EVENT_GET_TYPE(ev);
        pe.config = PERF_EVENT_GET_CONFIG(ev);
        fd = yy_perf_event_open(&pe, pid, cpu, group, flags);
        if (fd == -1) {
            if (perf->fd != -1) close(perf->fd);
            return false;
        }
        if (perf->fd == -1) perf->fd = fd;
        if (group == -1) group = fd; // add to same group
        ret = ioctl(fd, PERF_EVENT_IOC_ID, &perf->ids[i]);
        if (ret == -1) {
            close(perf->fd);
            return false;
        }
    }
    memset(perf->counters, 0, perf->count * sizeof(u64));
    perf->is_opened = true;
    return true;
}

bool yy_perf_close(yy_perf *perf) {
    if (!perf) return false;
    if (perf->is_counting) yy_perf_stop_counting(perf);
    if (perf->is_opened) {
        close(perf->fd);
        perf->is_opened = false;
        memset(perf->counters, 0, perf->count * sizeof(u64));
    }
    return true;
}

bool yy_perf_is_opened(yy_perf *perf) {
    return perf ? perf->is_opened : false;
}

bool yy_perf_start_counting(yy_perf *perf) {
    if (!perf) return false;
    if (!perf->is_opened) return false;
    if (perf->is_counting) return true;

    if (ioctl(perf->fd, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP) == -1) {
        return false;
    }
    if (ioctl(perf->fd, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP) == -1) {
        return false;
    }
    perf->is_counting = true;
    return true;
}

bool yy_perf_stop_counting(yy_perf *perf) {
    if (!perf) return false;
    if (!perf->is_counting) return false;

    perf->is_counting = false;
    if (ioctl(perf->fd, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP) == -1) return false;
    return true;
}

bool yy_perf_is_counting(yy_perf *perf) {
    return perf ? perf->is_counting : false;
}

u64 *yy_perf_get_counters(yy_perf *perf) {
    if (!perf) return false;
    if (!perf->is_opened) return false;

    // read counter data
    if (read(perf->fd, perf->buffer, (perf->count * 2 + 1) * sizeof(u64)) == -1) return false;
    
    // data layout: [count, val1, id1, val2, id2, ...]
    for (u32 i = 0; i < perf->count; i++) {
        perf->counters[i] = perf->buffer[i * 2 + 1];
    }
    return perf->counters;
}

#endif




/*==============================================================================
 * Apple
 *============================================================================*/

#if YY_APPLE_HAS_PERF

#include <dlfcn.h>

bool kperf_load(bool print_message_on_error) {
    static bool loaded = false;
    if (loaded) return true;
    
    void *kperf = dlopen("/System/Library/PrivateFrameworks/kperf.framework/kperf", RTLD_LAZY);
    if (!kperf) {
        if (print_message_on_error) {
            fprintf(stderr, "fail to load: kperf.framework, message: %s\n", dlerror());
        }
        return false;
    }
    void *kperfdata = dlopen("/System/Library/PrivateFrameworks/kperfdata.framework/kperfdata", RTLD_LAZY);
    if (!kperfdata) {
        if (print_message_on_error) {
            fprintf(stderr, "fail to load: kperfdata.framework, message: %s\n", dlerror());
        }
        return false;
    }
    
#define load_symbol(handle, symbol) \
    *(void **)&symbol = dlsym(handle, #symbol); \
    if (!symbol) { printf("fail to load: %s\n", #symbol); return false; }
    
    load_symbol(kperf, kpc_pmu_version);
    load_symbol(kperf, kpc_cpu_string);
    load_symbol(kperf, kpc_set_counting);
    load_symbol(kperf, kpc_get_counting);
    load_symbol(kperf, kpc_set_thread_counting);
    load_symbol(kperf, kpc_get_thread_counting);
    load_symbol(kperf, kpc_get_config_count);
    load_symbol(kperf, kpc_get_counter_count);
    load_symbol(kperf, kpc_set_config);
    load_symbol(kperf, kpc_get_config);
    load_symbol(kperf, kpc_get_cpu_counters);
    load_symbol(kperf, kpc_get_thread_counters);
    load_symbol(kperf, kpc_force_all_ctrs_set);
    load_symbol(kperf, kpc_force_all_ctrs_get);
    load_symbol(kperf, kperf_reset);
    
    load_symbol(kperfdata, kpep_config_create);
    load_symbol(kperfdata, kpep_config_free);
    load_symbol(kperfdata, kpep_config_add_event);
    load_symbol(kperfdata, kpep_config_remove_event);
    load_symbol(kperfdata, kpep_config_force_counters);
    load_symbol(kperfdata, kpep_config_events_count);
    load_symbol(kperfdata, kpep_config_events);
    load_symbol(kperfdata, kpep_config_kpc);
    load_symbol(kperfdata, kpep_config_kpc_count);
    load_symbol(kperfdata, kpep_config_kpc_classes);
    load_symbol(kperfdata, kpep_config_apply);
    load_symbol(kperfdata, kpep_db_create);
    load_symbol(kperfdata, kpep_db_free);
    load_symbol(kperfdata, kpep_db_name);
    load_symbol(kperfdata, kpep_db_architecture);
    load_symbol(kperfdata, kpep_db_aliases_count);
    load_symbol(kperfdata, kpep_db_aliases);
    load_symbol(kperfdata, kpep_db_counters_count);
    load_symbol(kperfdata, kpep_db_events_count);
    load_symbol(kperfdata, kpep_db_events);
    load_symbol(kperfdata, kpep_db_event);
    load_symbol(kperfdata, kpep_event_name);
    load_symbol(kperfdata, kpep_event_alias);
    load_symbol(kperfdata, kpep_event_description);
    load_symbol(kperfdata, kpep_event_errata);
    
    loaded = true;
    return true;
}

u32 (*kpc_pmu_version)(void);
int (*kpc_cpu_string)(char *buf, usize buf_size);
int (*kpc_set_counting)(u32 classes);
u32 (*kpc_get_counting)(void);
int (*kpc_set_thread_counting)(u32 classes);
u32 (*kpc_get_thread_counting)(void);
u32 (*kpc_get_config_count)(u32 classes);
u32 (*kpc_get_counter_count)(u32 classes);
int (*kpc_set_config)(u32 classes, u64 *buf_in);
int (*kpc_get_config)(u32 classes, u64 *buf_out);
int (*kpc_get_cpu_counters)(bool all_cpus, u32 classes, int *curcpu, u64 *buf);
int (*kpc_get_thread_counters)(u32 tid, u32 buf_count, u64 *buf);
int (*kpc_force_all_ctrs_set)(int val);
int (*kpc_force_all_ctrs_get)(int *val_out);
int (*kperf_reset)(void);

int (*kpep_config_create)(kpep_db *db, kpep_config **cfg_ptr);
void (*kpep_config_free)(kpep_config *cfg);
int (*kpep_config_add_event)(kpep_config *cfg, kpep_event **ev_ptr, u32 flag, u32 *err);
int (*kpep_config_remove_event)(kpep_config *cfg, usize idx);
int (*kpep_config_force_counters)(kpep_config *cfg);
int (*kpep_config_events_count)(kpep_config *cfg, usize *count_ptr);
int (*kpep_config_events)(kpep_config *cfg, kpep_event **buf, usize buf_count);
int (*kpep_config_kpc)(kpep_config *cfg, u64 *buf, usize buf_size);
int (*kpep_config_kpc_count)(kpep_config *cfg, usize *count_ptr);
int (*kpep_config_kpc_classes)(kpep_config *cfg, u32 *classes_ptr);
int (*kpep_config_apply)(kpep_config *cfg);
int (*kpep_db_create)(const char *cpu_name, kpep_db **db_ptr);
void (*kpep_db_free)(kpep_db *db);
int (*kpep_db_name)(kpep_db *db, const char **name);
int (*kpep_db_architecture)(kpep_db *db, u32 *arch);
int (*kpep_db_aliases_count)(kpep_db *db, usize *count);
int (*kpep_db_aliases)(kpep_db *db, const char **buf, usize buf_size);
int (*kpep_db_counters_count)(kpep_db *db, u8 classes, usize *count);
int (*kpep_db_events_count)(kpep_db *db, usize *count);
int (*kpep_db_events)(kpep_db *db, kpep_event **buf, usize buf_size);
int (*kpep_db_event)(kpep_db *db, const char *name, kpep_event **ev_ptr);
int (*kpep_event_name)(kpep_event *ev, const char **name_ptr);
int (*kpep_event_alias)(kpep_event *ev, const char **alias_ptr);
int (*kpep_event_description)(kpep_event *ev, const char **str_ptr);
int (*kpep_event_errata)(kpep_event *ev, const char **errata_ptr);




#include <TargetConditionals.h>

// configurable counters count (intel: 4, arm: 8)
#define MAX_COUNTER_NUM 8

static kpep_db *db = NULL;

struct yy_perf {
    kpep_config *cfg; // nonnull
    bool is_opened;
    bool is_counting;
    const char *names[MAX_COUNTER_NUM];
    u64 counters_begin[MAX_COUNTER_NUM];
    u64 counters_end[MAX_COUNTER_NUM];
    u64 counters_overhead[MAX_COUNTER_NUM];
    u64 counters[MAX_COUNTER_NUM];
};

static bool perf_event_to_name(yy_perf_event ev,
                               const char **name, const char **alias) {
#if TARGET_CPU_ARM64
    switch (ev) {
        case YY_PERF_EVENT_CYCLES:
            *name = "FIXED_CYCLES"; *alias = "cycles"; return true;
        case YY_PERF_EVENT_INSTRUCTIONS:
            *name = "FIXED_INSTRUCTIONS"; *alias = "instructions"; return true;
        case YY_PERF_EVENT_BRANCHES:
            *name = "INST_BRANCH"; *alias = "branches"; return true;
        case YY_PERF_EVENT_BRANCH_MISSES:
            *name = "BRANCH_MISPREDICT"; *alias = "branch-misses"; return true;
        case YY_PERF_EVENT_L1D_LOAD_MISSES:
            *name = "DCACHE_LOAD_MISS"; *alias = "L1d-load-misses"; return true;
        case YY_PERF_EVENT_L1D_STORE_MISSES:
            *name = "DCACHE_STORE_MISS"; *alias = "L1d-store-misses"; return true;
        default:
            return false;
    }
#elif TARGET_CPU_X86 || TARGET_CPU_X86_64
    switch (ev) {
        case YY_PERF_EVENT_CYCLES:
            *name = "CPU_CLK_UNHALTED.THREAD"; *alias = "cycles"; return true;
        case YY_PERF_EVENT_INSTRUCTIONS:
            *name = "INST_RETIRED.ANY"; *alias = "instructions"; return true;
        case YY_PERF_EVENT_BRANCHES:
            *name = "BR_INST_RETIRED.ALL_BRANCHES"; *alias = "branches"; return true;
        case YY_PERF_EVENT_BRANCH_MISSES:
            *name = "BR_MISP_RETIRED.ALL_BRANCHES"; *alias = "branch-misses"; return true;
        default:
            return false;
    }
#else
    return false;
#endif
}

static yy_inline void perf_calc_counters(yy_perf *perf) {
    for (i32 i = 0; i < MAX_COUNTER_NUM; i++) {
        u64 begin = perf->counters_begin[i];
        u64 end = perf->counters_end[i];
        u64 overhead = perf->counters_overhead[i];
        u64 delta = end - begin;
        u64 count = delta < overhead ? 0 : delta - overhead;
        perf->counters[i] = count;
    }
}

bool yy_perf_load(bool print_message_on_error) {
    bool suc = kperf_load(print_message_on_error);
    if (!suc) return false;
    if (db) return true;
    
    int ret = kpep_db_create(NULL, &db);
    if (ret != 0) {
        if (print_message_on_error) {
            fprintf(stderr, "fail to load kpep db for host CPU, error: %d.\n", ret);
        }
        return false;
    }
    
    u32 ver = kpc_pmu_version();
    if (ver == KPC_PMU_ERROR) {
        if (print_message_on_error) {
            fprintf(stderr, "Cannot load kperf, this requires root privileges (or blessed).\n");
        }
        return false;
    }
    
    return true;
}

yy_perf *yy_perf_new(void) {
    if (!db) return NULL;
    
    yy_perf *perf = (yy_perf *)calloc(1, sizeof(yy_perf));
    if (!perf) return false;
    
    int ret = kpep_config_create(db, &perf->cfg);
    if (ret != 0) {
        free(perf);
        return false;
    }
    return perf;
}

void yy_perf_free(yy_perf *perf) {
    if (!perf) return;
    yy_perf_close(perf);
    kpep_config_free(perf->cfg);
    free(perf);
}

bool yy_perf_add_event(yy_perf *perf, yy_perf_event event) {
    if (!perf) return false;
    const char *name, *alias;
    if (!perf_event_to_name(event, &name, &alias)) return false;
    return yy_perf_add_event_apple(perf, name, alias);
}

bool yy_perf_remove_all_events(yy_perf *perf) {
    if (!perf) return false;
    while (true) {
        usize count = 0;
        if (kpep_config_events_count(perf->cfg, &count) != 0) return false;
        if (count == 0) return true;
        if (kpep_config_remove_event(perf->cfg, 0) != 0) return false;
    }
}

bool yy_perf_event_available(yy_perf *perf, yy_perf_event ev) {
    const char *name, *alias;
    if (!perf_event_to_name(ev, &name, &alias)) return false;
    return true;
}

bool yy_perf_add_event_apple(yy_perf *perf, const char *ev_name, const char *ev_alias) {
    if (!perf || !ev_name) return false;
    
    kpep_event *ev;
    if (kpep_db_event(db, ev_name, &ev) != 0) return false;
    
    usize count = 0;
    if (kpep_config_events_count(perf->cfg, &count) != 0) return false;
    if (kpep_config_add_event(perf->cfg, &ev, 1, NULL) != 0) return false;
    perf->names[count] = ev_alias ? ev_alias : ev_name;
    return true;
}

bool yy_perf_event_available_apple(yy_perf *perf, const char *ev_name) {
    if (!db || !ev_name) return false;
    
    kpep_event *ev;
    if (kpep_db_event(db, ev_name, &ev) != 0) return false;
    return true;
}

bool yy_perf_add_event_linux(yy_perf *perf, u64 ev_value, const char *ev_alias) {
    return false;
}

bool yy_perf_event_available_linux(yy_perf *perf, u64 ev_value) {
    return false;
}

u32 yy_perf_get_event_count(yy_perf *perf) {
    if (!perf) return 0;
    usize count = 0;
    if (kpep_config_events_count(perf->cfg, &count) != 0) return 0;
    return (u32)count;
}

const char **yy_perf_get_event_names(yy_perf *perf) {
    return perf ? perf->names : NULL;
}

bool yy_perf_open(yy_perf *perf) {
    u32 count = yy_perf_get_event_count(perf);
    if (count == 0) return false;
    
    i32 ret;
    
    u32 classes;
    ret = kpep_config_kpc_classes(perf->cfg, &classes);
    if (ret != 0) return false;
    
    ret = kpep_config_apply(perf->cfg);
    if (ret != 0) return false;
    
    ret = kpc_set_counting(classes);
    if (ret != 0) return false;
    
    ret = kpc_set_thread_counting(classes);
    if (ret != 0) return false;
    
    memset(perf->counters_begin, 0, sizeof(perf->counters));
    memset(perf->counters_end, 0, sizeof(perf->counters_end));
    memset(perf->counters_overhead, 0, sizeof(perf->counters_overhead));
    memset(perf->counters, 0, sizeof(perf->counters));
    perf->is_opened = true;
    
    yy_perf_start_counting(perf);
    yy_perf_stop_counting(perf);
    memcpy(perf->counters_overhead, yy_perf_get_counters(perf), sizeof(perf->counters));
    
    return true;
}

bool yy_perf_close(yy_perf *perf) {
    if (!perf) return false;
    i32 ret;
    
    ret = kpc_set_counting(0);
    if (ret != 0) return false;
    
    ret = kpc_set_thread_counting(0);
    if (ret != 0) return false;
    
    u64 buf[64] = {0};
    ret = kpc_set_config(0, buf);
    if (ret != 0) return false;
    
    return true;
}

bool yy_perf_is_opened(yy_perf *perf) {
    return perf ? perf->is_opened : false;
}

bool yy_perf_start_counting(yy_perf *perf) {
    if (!perf || !perf->is_opened) return false;
    if (perf->is_counting) return true;
    
    memset(perf->counters_begin, 0, sizeof(perf->counters));
    memset(perf->counters_end, 0, sizeof(perf->counters_end));
    memset(perf->counters, 0, sizeof(perf->counters));
    
    i32 ret = kpc_get_thread_counters(0, MAX_COUNTER_NUM, perf->counters_begin);
    if (ret != 0) return false;
    
    memcpy(perf->counters_end, perf->counters_begin, MAX_COUNTER_NUM);
    
    perf->is_counting = true;
    return true;
}

bool yy_perf_stop_counting(yy_perf *perf) {
    if (!perf || !perf->is_counting) return false;
    
    i32 ret = kpc_get_thread_counters(0, MAX_COUNTER_NUM, perf->counters_end);
    if (ret != 0) return false;
    perf_calc_counters(perf);
    perf->is_counting = false;
    return true;
}

bool yy_perf_is_counting(yy_perf *perf) {
    return perf ? perf->is_counting : false;
}

u64 *yy_perf_get_counters(yy_perf *perf) {
    if (!perf) return NULL;
    
    if (perf->is_counting) {
        i32 ret = kpc_get_thread_counters(0, MAX_COUNTER_NUM, perf->counters_end);
        if (ret != 0) return false;
        perf_calc_counters(perf);
    }
    
    return perf->counters;
}


#endif








/*==============================================================================
 * Dummy
 *============================================================================*/

#if !YY_PERF_AVAILABLE

struct yy_perf {
    int dummy;
};

bool yy_perf_load(bool print_message_on_error) {
    if (print_message_on_error) {
        fprintf(stderr, "perf module does'n support this platform.\n");
    }
    return false;
}

yy_perf *yy_perf_new(void) {
    return (yy_perf *)calloc(1, sizeof(yy_perf));
}

void yy_perf_free(yy_perf *perf) {
    if (perf) free(perf);
}

bool yy_perf_add_event(yy_perf *perf, yy_perf_event event) {
    return false;
}

bool yy_perf_remove_all_events(yy_perf *perf) {
    return false;
}

bool yy_perf_event_available(yy_perf *perf, yy_perf_event ev) {
    return false;
}

bool yy_perf_add_event_apple(yy_perf *perf, const char *ev_name, const char *ev_alias) {
    return false;
}

bool yy_perf_event_available_apple(yy_perf *perf, const char *ev_name) {
    return false;
}

bool yy_perf_add_event_linux(yy_perf *perf, u64 ev_value, const char *ev_alias) {
    return false;
}

bool yy_perf_event_available_linux(yy_perf *perf, u64 ev_value) {
    return false;
}

u32 yy_perf_get_event_count(yy_perf *perf) {
    return 0;
}

const char **yy_perf_get_event_names(yy_perf *perf) {
    return NULL;
}

bool yy_perf_open(yy_perf *perf) {
    return false;
}

bool yy_perf_close(yy_perf *perf) {
    return false;
}

bool yy_perf_is_opened(yy_perf *perf) {
    return false;
}

bool yy_perf_start_counting(yy_perf *perf) {
    return false;
}

bool yy_perf_stop_counting(yy_perf *perf) {
    return false;
}

bool yy_perf_is_counting(yy_perf *perf) {
    return false;
}

u64 *yy_perf_get_counters(yy_perf *perf) {
    return NULL;
}

#endif
