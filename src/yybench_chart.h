/*==============================================================================
 * Copyright (C) 2020 YaoYuan <ibireme@gmail.com>.
 * Released under the MIT license (MIT).
 *============================================================================*/

#ifndef yybench_chart_h
#define yybench_chart_h

#include "yybench_def.h"
#include "yybench_str.h"

#ifdef __cplusplus
extern "C" {
#endif


/*==============================================================================
 * Chart
 *============================================================================*/

/*
 It use Highcharts (Not-for-Profit usage license) to display charts.
 If you need to use it commercially, you need to obtain a new license,
 or you can change it with another opensource chart library.
 
 Code Example:
 
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
    
    // Write and free the report
    yy_report_write_html_file(report, "report.html");
    yy_report_free(report);
 */


/** Chart options enum */
typedef enum {
    /* Chart type */
    YY_CHART_LINE,   /* line chart */
    YY_CHART_BAR,    /* bar chart */
    YY_CHART_COLUMN, /* column chart */
    YY_CHART_PIE,    /* pie chart */
    YY_CHART_TABLE,  /* table (not a chart) */
    
    /* Legend layout */
    YY_CHART_HORIZONTAL, /* horizontal layout */
    YY_CHART_VERTICAL,   /* vertical layout */
    YY_CHART_PROXIMATE,  /* close to the graphs they're representing */
    
    /* Horizontal alignment */
    YY_CHART_LEFT,       /* align to left */
    YY_CHART_CENTER,     /* align to center */
    YY_CHART_RIGHT,      /* align to right */
    
    /* Vertical alignment */
    YY_CHART_TOP,        /* align to top */
    YY_CHART_MIDDLE,     /* align to middle */
    YY_CHART_BOTTOM      /* align to bottom */
} yy_chart_enum;

/** Chart axis options */
typedef struct {
    const char *title; /* axis title, default is NULL */
    const char *label_prefix; /* a string to prepend to each axis label, default is NULL */
    const char *label_suffix; /* a string to append to each axis label, default is NULL */
    float min, max; /* the min/max value of the axis, default is NaN */
    float tick_interval; /* the interval of the tick marks in axis units, default is NaN */
    bool allow_decimals; /* whether to allow decimals in this axis' ticks, default is true */
    bool logarithmic; /* type of axis (linear or logarithmic), default is false (linear) */
    const char **categories; /* If categories are present for this axis, names are used instead of
                              numbers for this axis. For example: {"Q1", "Q2", "Q3", "Q4", NULL}; */
} yy_chart_axis_options;

/** Chart tooltip options */
typedef struct {
    int value_decimals; /* how many decimals to show in each value, default is -1 (preserve all decimals) */
    const char *value_prefix; /* a string to prepend to each series' value, default is NULL */
    const char *value_suffix; /* a string to append to each series' value, default is NULL */
    bool shared; /* the entire plot area will capture mouse movement, default is false */
    bool crosshairs; /* enable a crosshair for the value, default is false */
} yy_chart_tooltip_options;

/** Chart legend options */
typedef struct {
    bool enabled; /* enable or disable the legend, default is true */
    yy_chart_enum layout; /* the layout of the legend items, default is vertical */
    yy_chart_enum h_align; /* the horizontal alignment of the legend box, default is right */
    yy_chart_enum v_align; /* the vertical alignment of the legend box, default is middle */
} yy_chart_legend_options;

/** Chart plot options */
typedef struct {
    bool name_label_enabled; /* enable the name label per item, default is false */
    bool value_labels_enabled; /* enable all value labels, default is false */
    int value_labels_decimals; /* how many decimals to show in each value, default is -1 */
    /* Options for line chart */
    float point_start;    /* the start of the x values, default is 0 */
    float point_interval; /* the interval of the x values, default is 1 */
    /* Options for bar and column chart */
    bool color_by_point; /* one color per item (group), default is false */
    bool group_stacked; /* stack the values of each item (group), default is false */
    float group_padding; /* padding between each item (group), default is 0.2 */
    float point_padding; /* padding between each column or bar, default is 0.1 */
    float border_width; /* the width of the border surrounding each column or bar, default is 1 */
} yy_chart_plot_options;

/** Chart options */
typedef struct {
    yy_chart_enum type; /* chart type, default is 'line' */
    int width, height; /* chart size, default is 800*500 */
    const char *title; /* title for chart */
    const char *subtitle; /* subtitle for chart */
    const char **colors; /* color pattern, for example {"#058DC7", "#50B432", "#ED561B", "#DDDF00", NULL} */
    yy_chart_axis_options v_axis; /* vertical axis options */
    yy_chart_axis_options h_axis; /* horizontal axis options */
    yy_chart_tooltip_options tooltip; /* tooltip options */
    yy_chart_legend_options legend; /* legend options */
    yy_chart_plot_options plot; /* plot options */
} yy_chart_options;

/** Set chart options to default value */
void yy_chart_options_init(yy_chart_options *op);


/** A chart object */
typedef struct yy_chart yy_chart;

/** Create a new chart object. */
yy_chart *yy_chart_new(void);

/** Release the chart object. */
void yy_chart_free(yy_chart *chart);

/** Set chart options, the options was copied. */
bool yy_chart_set_options(yy_chart *chart, yy_chart_options *op);

/** Begin a chart item */
bool yy_chart_item_begin(yy_chart *chart, const char *name);

/** Add an integer value to current chart item */
bool yy_chart_item_add_int(yy_chart *chart, int value);

/** Add a floating value to current chart item */
bool yy_chart_item_add_float(yy_chart *chart, float value);

/** End a chart item */
bool yy_chart_item_end(yy_chart *chart);

/** Same as item_begin(name); item_add_int(value), item_end(); */
bool yy_chart_item_with_int(yy_chart *chart, const char *name, int value);

/** Same as item_begin(name); item_add_float(value), item_end(); */
bool yy_chart_item_with_float(yy_chart *chart, const char *name, float value);

/** Sort items with average values (ascent or descent) */
bool yy_chart_sort_items_with_value(yy_chart *chart, bool ascent);

/** Sort items with name (ascent or descent) */
bool yy_chart_sort_items_with_name(yy_chart *chart, bool ascent);


/*==============================================================================
 * HTML Report
 *============================================================================*/

/** A report object */
typedef struct yy_report yy_report;

/** Creates a report. */
yy_report *yy_report_new(void);

/** Release a report. */
void yy_report_free(yy_report *report);

/** Add a chart to report. */
bool yy_report_add_chart(yy_report *report, yy_chart *chart);

/** Add a text information to report. */
bool yy_report_add_info(yy_report *report, const char *info);

/** Add environment information to report. */
bool yy_report_add_env_info(yy_report *report);

/** Write the report to html string, should be released with free(). */
bool yy_report_write_html_string(yy_report *report, char **html, usize *len);

/** Write the report to html file. */
bool yy_report_write_html_file(yy_report *report, const char *path);


#ifdef __cplusplus
}
#endif

#endif
