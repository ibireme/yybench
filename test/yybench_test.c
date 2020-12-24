/*==============================================================================
 * Copyright (C) 2020 YaoYuan <ibireme@gmail.com>.
 * Released under the MIT license (MIT).
 *============================================================================*/

#include "yybench.h"


static void test_env(void) {
    printf("prepare...\n");
    yy_cpu_measure_freq();
    printf("OS: %s\n", yy_env_get_os_desc());
    printf("Compiler: %s\n", yy_env_get_compiler_desc());
    printf("CPU: %s\n", yy_env_get_cpu_desc());
    printf("CPU Freq: %.2f MHz\n", yy_cpu_get_freq() / 1000.0 / 1000.0);
    printf("\n");
}


static void test_perf(void) {
    printf("pref test:\n");
    if (yy_perf_load(true) == false) return;
    
    // create perf object
    yy_perf *perf = yy_perf_new();
    
    // add event and open perf
    yy_perf_add_event(perf, YY_PERF_EVENT_CYCLES);
    yy_perf_add_event(perf, YY_PERF_EVENT_INSTRUCTIONS);
    yy_perf_add_event(perf, YY_PERF_EVENT_BRANCHES);
    yy_perf_add_event(perf, YY_PERF_EVENT_BRANCH_MISSES);
    if (!yy_perf_open(perf)) {
        printf("perf open fail\n");
        return;
    }
    
    // profile
    yy_perf_start_counting(perf);
    u64 t1 = yy_time_get_ticks();
    volatile int add = 0;
    for (int i = 0; i < 100000000; i++) {
        add++;
    }
    u64 t2 = yy_time_get_ticks();
    yy_perf_stop_counting(perf);
    
    // get result
    u32 count = yy_perf_get_event_count(perf);
    const char **names = yy_perf_get_event_names(perf);
    u64 *counters = yy_perf_get_counters(perf);
    
    for (u32 i = 0; i < count; i++) {
        printf("%d. %.14s: %llu\n",i, names[i], counters[i]);
    }
    
    // tick
    u64 tick = t2 - t1;
    f64 time = yy_cpu_tick_to_sec(tick);
    printf("time: %.3f ms\n", time * 1000.0);
    printf("Cycles: %llu(PMU), %llu(Tick), accuracy:%.3f%%\n",
           counters[0], yy_cpu_tick_to_cycle(tick),
           (f64)yy_cpu_tick_to_cycle(tick) / counters[0] * 100);
    printf("IPC: %.3f\n", (f64)counters[1] / (f64)counters[0]);
    
    // close perf and free resources
    yy_perf_close(perf);
    yy_perf_free(perf);
    
    printf("\n");
}


static void test_chart(void) {
    // Create a report, add some infos.
    yy_report *report = yy_report_new();
    yy_report_add_info(report, "This is a report demo");
    yy_report_add_info(report, "The chart is rendered with highcharts");
    yy_report_add_env_info(report);
    
    {
        // Config line chart options.
        yy_chart_options op;
        yy_chart_options_init(&op);
        op.title = "Line Chart Demo";
        op.type = YY_CHART_LINE;
        op.v_axis.title = "this is v axis";
        op.h_axis.title = "this is h axis";
        op.tooltip.value_decimals = 3;
        
        // Create a chart and set options.
        yy_chart *chart = yy_chart_new();
        yy_chart_set_options(chart, &op);
        
        // Add a line to chart.
        yy_chart_item_begin(chart, "sin line");
        for (float i = 0; i < M_PI * 2; i += 0.1f) {
            yy_chart_item_add_float(chart, sinf(i));
        }
        yy_chart_item_end(chart);
        
        // Add a line to chart.
        yy_chart_item_begin(chart, "cos line");
        for (float i = 0; i < M_PI * 2; i += 0.1f) {
            yy_chart_item_add_float(chart, cosf(i));
        }
        yy_chart_item_end(chart);
        
        // Add chart to report, and free the chart.
        yy_report_add_chart(report, chart);
        yy_chart_free(chart);
    }
    
    {
        // Config bar chart options.
        yy_chart_options op;
        yy_chart_options_init(&op);
        op.title = "Bar Chart Demo";
        op.type = YY_CHART_BAR;
        op.v_axis.title = "this is v axis";
        op.h_axis.title = "this is h axis";
        const char *categories[] = {"Q1", "Q2", "Q3", "Q4", NULL};;
        op.v_axis.categories = categories;
        
        // Create a chart and set options.
        yy_chart *chart = yy_chart_new();
        yy_chart_set_options(chart, &op);
        
        // Add a bar group to chart.
        yy_chart_item_begin(chart, "year 2019");
        yy_chart_item_add_int(chart, 20);
        yy_chart_item_add_int(chart, 25);
        yy_chart_item_add_int(chart, 30);
        yy_chart_item_add_int(chart, 15);
        yy_chart_item_end(chart);
        
        // Add a bar group to chart.
        yy_chart_item_begin(chart, "year 2020");
        yy_chart_item_add_int(chart, 20);
        yy_chart_item_add_int(chart, 30);
        yy_chart_item_add_int(chart, 45);
        yy_chart_item_add_int(chart, 25);
        yy_chart_item_end(chart);
        
        // Add chart to report, and free the chart.
        yy_report_add_chart(report, chart);
        yy_chart_free(chart);
    }
    
    {
        // Config table options.
        yy_chart_options op;
        yy_chart_options_init(&op);
        op.title = "Sortable Table Demo";
        op.type = YY_CHART_TABLE;
        static const char *categories[] = {"Q1", "Q2", "Q3", "Q4", NULL};;
        op.h_axis.categories = categories;
        
        // Create a chart and set options.
        yy_chart *chart = yy_chart_new();
        yy_chart_set_options(chart, &op);
        
        // Add a line to table.
        yy_chart_item_begin(chart, "year 2018");
        yy_chart_item_add_int(chart, 10);
        yy_chart_item_add_int(chart, 10);
        yy_chart_item_add_int(chart, 10);
        yy_chart_item_add_int(chart, 10);
        yy_chart_item_end(chart);
        
        // Add a line to table.
        yy_chart_item_begin(chart, "year 2019");
        yy_chart_item_add_int(chart, 20);
        yy_chart_item_add_int(chart, 25);
        yy_chart_item_add_int(chart, 30);
        yy_chart_item_add_int(chart, 15);
        yy_chart_item_end(chart);
        
        // Add a line to table.
        yy_chart_item_begin(chart, "year 2020");
        yy_chart_item_add_int(chart, 20);
        yy_chart_item_add_int(chart, 30);
        yy_chart_item_add_int(chart, 45);
        yy_chart_item_add_int(chart, 25);
        yy_chart_item_end(chart);
        
        // Add chart to report, and free the chart.
        yy_report_add_chart(report, chart);
        yy_chart_free(chart);
    }
    
    
    if (yy_perf_load(false)) {
        // branch misprediction penalty
        printf("test branch misprediction penalty...\n");
        
#define ITERAT_NUM 128
#define SAMPLE_NUM 200
#define BRANCH_NUM 4096
        float cycles[SAMPLE_NUM + 1] = {0};
        float misses[SAMPLE_NUM + 1] = {0};
        
        yy_perf *perf = yy_perf_new();
        yy_perf_add_event(perf, YY_PERF_EVENT_CYCLES);
        yy_perf_add_event(perf, YY_PERF_EVENT_BRANCH_MISSES);
        yy_perf_open(perf);
        for (int iter = 0; iter < ITERAT_NUM; iter++) {
            for (int s = 0; s <= SAMPLE_NUM; s++) {
                yy_perf_start_counting(perf);
                for (int i = 0; i < BRANCH_NUM; i++) {
                    if ((int)(yy_random32() % SAMPLE_NUM) < s) {
                        yy_random32();
                        yy_random32();
                        yy_random32();
                        yy_random32();
                    } else {
                        yy_random64();
                        yy_random64();
                    }
                }
                yy_perf_stop_counting(perf);
                
                u64 *vals = yy_perf_get_counters(perf);
                u64 cycle = vals[0];
                u64 miss = vals[1];
                cycles[s] += cycle;
                misses[s] += miss;
            }
        }
        float cycle_min = cycles[0];
        float cycle_max = cycles[SAMPLE_NUM];
        for (int s = 0; s <= SAMPLE_NUM; s++) {
            cycles[s] -= cycle_min + (cycle_max - cycle_min) * s / SAMPLE_NUM;
            cycles[s] /= BRANCH_NUM * ITERAT_NUM;
            misses[s] /= BRANCH_NUM * ITERAT_NUM;
        }
        yy_perf_free(perf);
        
        
        // Config line chart options.
        yy_chart_options op;
        yy_chart_options_init(&op);
        op.width = 600;
        op.height = 400;
        op.title = "CPU Branch Misprediction Penalty";
        op.type = YY_CHART_LINE;
        op.h_axis.title = "random";
        op.tooltip.value_decimals = 3;
        
        // Create a chart and set options.
        yy_chart *chart = yy_chart_new();
        yy_chart_set_options(chart, &op);
        
        yy_chart_item_begin(chart, "cycles");
        for (int i = 0; i <= SAMPLE_NUM; i ++) {
            yy_chart_item_add_float(chart, cycles[i]);
        }
        yy_chart_item_end(chart);
        
        yy_chart_item_begin(chart, "miss rate");
        for (int i = 0; i <= SAMPLE_NUM; i ++) {
            yy_chart_item_add_float(chart, misses[i]);
        }
        yy_chart_item_end(chart);
        
        yy_chart_item_begin(chart, "penalty");
        for (int i = 0; i <= SAMPLE_NUM; i ++) {
            float cycle = cycles[i];
            float miss = misses[i];
            float penalty = cycle / miss;
            if (!isfinite(penalty)) penalty = 0;
            penalty = penalty < 0 ? 0 : penalty > 50 ? 50 : penalty;
            yy_chart_item_add_float(chart, penalty);
        }
        yy_chart_item_end(chart);
        
        
        // Add chart to report, and free the chart.
        yy_report_add_chart(report, chart);
        yy_chart_free(chart);
        
        
        
    }
    
    
    // Write and free the report
    const char *path = "report.html";
    yy_report_write_html_file(report, path);
    yy_report_free(report);
    printf("write demo chart report to: %s\n", path);
}


int main(void) {
    test_env();
    test_perf();
    test_chart();
}
