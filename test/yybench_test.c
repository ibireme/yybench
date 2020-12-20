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
    yy_perf_open(perf);
    
    // profile
    yy_perf_start_counting(perf);
    volatile int add = 0;
    for (int i = 0; i < 100000; i++) {
        add++;
    }
    yy_perf_stop_counting(perf);
    
    // get result
    u32 count = yy_perf_get_event_count(perf);
    const char **names = yy_perf_get_event_names(perf);
    u64 *counters = yy_perf_get_counters(perf);
    
    for (u32 i = 0; i < count; i++) {
        printf("%d. %.14s: %llu\n",i, names[i], counters[i]);
    }
    
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
        op.height = 0; // auto height
        const char *categories[] = {"Q1", "Q2", "Q3", "Q4", NULL};;
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
