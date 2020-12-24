/*==============================================================================
 * Copyright (C) 2020 YaoYuan <ibireme@gmail.com>.
 * Released under the MIT license (MIT).
 *============================================================================*/

#include "yybench_chart.h"
#include "yybench_env.h"
#include "yybench_cpu.h"
#include "yybench_file.h"

#define ARR_TYPE(type) yy_buf
#define ARR_INIT(arr) yy_buf_init(&(arr), 0)
#define ARR_RELEASE(arr) yy_buf_release(&(arr))
#define ARR_HEAD(arr, type) (((type *)(&(arr))->hdr))
#define ARR_GET(arr, type, idx) (((type *)(&(arr))->hdr) + idx)
#define ARR_COUNT(arr, type) (yy_buf_len(&(arr)) / sizeof(type))
#define ARR_ADD(arr, value, type) (yy_buf_append(&(arr), (void *)&(value), sizeof(type)))

typedef struct {
    f64 v;
    bool is_integer;
    bool is_null;
} yy_chart_value;

typedef struct {
    const char *name;
    ARR_TYPE(yy_chart_value) values;
    f64 avg_value;
} yy_chart_item;

struct yy_chart {
    yy_chart_options options;
    ARR_TYPE(yy_chart_item) items;
    bool item_opened;
    bool options_need_release;
    int ref_count;
};

void yy_chart_options_init(yy_chart_options *op) {
    if (!op) return;
    memset(op, 0, sizeof(yy_chart_options));
    op->type = YY_CHART_LINE;
    op->width = 800;
    op->height = 500;
    
    op->h_axis.min = op->h_axis.max = op->h_axis.tick_interval = NAN;
    op->h_axis.allow_decimals = true;
    op->v_axis.min = op->v_axis.max = op->v_axis.tick_interval = NAN;
    op->v_axis.allow_decimals = true;

    op->tooltip.value_decimals = -1;
    
    op->legend.enabled = true;
    op->legend.layout = YY_CHART_VERTICAL;
    op->legend.h_align = YY_CHART_RIGHT;
    op->legend.v_align = YY_CHART_MIDDLE;
    
    op->plot.point_interval = 1;
    op->plot.group_padding = 0.2f;
    op->plot.point_padding = 0.1f;
    op->plot.border_width = 1.0f;
    op->plot.value_labels_decimals = -1;
}

static int yy_chart_axis_category_count(yy_chart_axis_options *op) {
    int i;
    if (!op || !op->categories) return 0;
    for (i = 0; op->categories[i]; i++) {}
    return i;
}

static void yy_chart_axis_options_release(yy_chart_axis_options *op) {
    if (!op) return;
    if (op->title) free((void *)op->title);
    if (op->label_prefix) free((void *)op->label_prefix);
    if (op->label_suffix) free((void *)op->label_suffix);
    if (op->categories) {
        int i;
        for (i = 0; op->categories[i]; i++) {
            free((void *)op->categories[i]);
        }
        free((void *)op->categories);
    }
}

static bool yy_chart_axis_options_copy(yy_chart_axis_options *dst,
                                       yy_chart_axis_options *src) {
#define RETURN_FAIL() do { yy_chart_axis_options_release(dst); return false;} while (0)
#define STR_COPY(name) do { \
    if (src->name) { \
        dst->name = yy_str_copy(src->name); \
        if (!(dst->name)) RETURN_FAIL(); \
    }  } while(0)
    if (!src || !dst) return false;
    STR_COPY(title);
    STR_COPY(label_prefix);
    STR_COPY(label_suffix);
    if (src->categories) {
        int i, count;
        count = yy_chart_axis_category_count(src);
        if (count) {
            dst->categories = calloc(count + 1, sizeof(char *));
            if (!dst->categories) RETURN_FAIL();
            for (i = 0 ; i < count; i++) STR_COPY(categories[i]);
        }
    }
    return true;
#undef STR_COPY
#undef RETURN_FAIL
}

static void yy_chart_options_release(yy_chart_options *op) {
#define STR_FREE(name) if (op->name) free((void *)op->name)
    if (!op) return;
    STR_FREE(title);
    STR_FREE(subtitle);
    STR_FREE(tooltip.value_prefix);
    STR_FREE(tooltip.value_suffix);
    yy_chart_axis_options_release(&op->h_axis);
    yy_chart_axis_options_release(&op->v_axis);
#undef STR_FREE
}

static bool yy_chart_options_copy(yy_chart_options *dst,
                                  yy_chart_options *src) {
#define RETURN_FAIL() do { yy_chart_options_release(dst); return false;} while (0)
#define STR_COPY(name) do { \
    if (src->name) { \
        dst->name = yy_str_copy(src->name); \
        if (!(dst->name)) RETURN_FAIL(); \
    } } while(0)
    if (!src || !dst) return false;
    *dst = *src;
    STR_COPY(title);
    STR_COPY(subtitle);
    STR_COPY(tooltip.value_prefix);
    STR_COPY(tooltip.value_suffix);
    if (!yy_chart_axis_options_copy(&dst->h_axis, &src->h_axis)) RETURN_FAIL();
    if (!yy_chart_axis_options_copy(&dst->v_axis, &src->v_axis)) RETURN_FAIL();
    return true;
#undef STR_COPY
#undef RETURN_FAIL
}

static bool yy_chart_item_release(yy_chart_item *item) {
    if (!item) return false;
    if (item->name) free((void *)item->name);
    ARR_RELEASE(item->values);
    return true;
}

static bool yy_chart_item_init(yy_chart_item *item, const char *name) {
    if (!item || !name) return false;
    memset(item, 0, sizeof(yy_chart_item));
    if (!ARR_INIT(item->values)) return false;
    item->name = yy_str_copy(name);
    if (!item->name) {
        ARR_RELEASE(item->values);
        return false;
    }
    return true;
}

yy_chart *yy_chart_new(void) {
    yy_chart *chart = calloc(1, sizeof(struct yy_chart));
    if (!chart) return NULL;
    yy_chart_options_init(&chart->options);
    chart->ref_count = 1;
    return chart;
}

void yy_chart_free(yy_chart *chart) {
    if (!chart) return;
    chart->ref_count--;
    if (chart->ref_count > 0) return;
    if (chart->options_need_release) yy_chart_options_release(&chart->options);
    free((void *)chart);
}

bool yy_chart_set_options(yy_chart *chart, yy_chart_options *op) {
    if (!chart || !op) return false;
    if (chart->options_need_release) {
        yy_chart_options_release(&chart->options);
    }
    if (yy_chart_options_copy(&chart->options, op)) {
        chart->options_need_release = true;
        return true;
    } else {
        yy_chart_options_init(&chart->options);
        chart->options_need_release = false;
        return false;
    }
}

bool yy_chart_item_begin(yy_chart *chart, const char *name) {
    yy_chart_item item;
    
    if (!chart || !name) return false;
    if (chart->item_opened) return false;
    
    if (!yy_chart_item_init(&item, name)) return false;
    if (!ARR_ADD(chart->items, item, yy_chart_item)) {
        yy_chart_item_release(&item);
        return false;
    }
    chart->item_opened = true;
    return true;
}

bool yy_chart_item_add_int(yy_chart *chart, int value) {
    size_t count;
    yy_chart_item *item;
    yy_chart_value cvalue;
    
    if (!chart || !chart->item_opened) return false;
    cvalue.v = value;
    cvalue.is_integer = true;
    cvalue.is_null = false;
    count = ARR_COUNT(chart->items, yy_chart_item);
    item = ARR_GET(chart->items, yy_chart_item, count - 1);
    return ARR_ADD(item->values, cvalue, yy_chart_value);
}

bool yy_chart_item_add_float(yy_chart *chart, float value) {
    size_t count;
    yy_chart_item *item;
    yy_chart_value cvalue;
    
    if (!chart || !chart->item_opened) return false;
    cvalue.v = value;
    cvalue.is_integer = false;
    cvalue.is_null = !isfinite(cvalue.v);
    count = ARR_COUNT(chart->items, yy_chart_item);
    item = ARR_GET(chart->items, yy_chart_item, count - 1);
    return ARR_ADD(item->values, cvalue, yy_chart_value);
}

bool yy_chart_item_end(yy_chart *chart) {
    if (!chart) return false;
    if (!chart->item_opened) return false;
    chart->item_opened = false;
    return true;
}

bool yy_chart_item_with_int(yy_chart *chart, const char *name, int value) {
    if (!yy_chart_item_begin(chart, name) ||
        !yy_chart_item_add_int(chart, value) ||
        !yy_chart_item_end(chart)) return false;
    return true;
}

bool yy_chart_item_with_float(yy_chart *chart, const char *name, float value) {
    if (!yy_chart_item_begin(chart, name) ||
        !yy_chart_item_add_float(chart, value) ||
        !yy_chart_item_end(chart)) return false;
    return true;
}

static int yy_chart_item_cmp_value_asc(const void *p1, const void *p2) {
    f64 v1, v2;
    v1 = ((yy_chart_item *)p1)->avg_value;
    v2 = ((yy_chart_item *)p2)->avg_value;
    if (v1 == v2) return 0;
    return v1 < v2 ? -1 : 1;
}

static int yy_chart_item_cmp_value_desc(const void *p1, const void *p2) {
    f64 v1, v2;
    v1 = ((yy_chart_item *)p1)->avg_value;
    v2 = ((yy_chart_item *)p2)->avg_value;
    if (v1 == v2) return 0;
    return v1 > v2 ? -1 : 1;
}

bool yy_chart_sort_items_with_value(yy_chart *chart, bool ascent) {
    size_t i, imax, j, jmax, count;
    f64 sum;
    
    for ((void)(i = 0), imax = ARR_COUNT(chart->items, yy_chart_item); i < imax; i++) {
        yy_chart_item *item = ARR_GET(chart->items, yy_chart_item, i);
        count = 0;
        sum = 0;
        for ((void)(j = 0), jmax = ARR_COUNT(item->values, yy_chart_value); j < jmax; j++) {
            yy_chart_value *value = ARR_GET(item->values, yy_chart_value, j);
            if (!value->is_null) {
                count++;
                sum += value->v;
            }
        }
        item->avg_value = count ? sum / count : 0;
    }
    
    if (imax <= 1) return true;
    qsort(chart->items.hdr, imax, sizeof(yy_chart_item),
          ascent ? yy_chart_item_cmp_value_asc : yy_chart_item_cmp_value_desc);
    return true;
}

static int yy_chart_item_cmp_name_asc(const void *p1, const void *p2) {
    return strcmp(((yy_chart_item *)p1)->name, ((yy_chart_item *)p2)->name);
}

static int yy_chart_item_cmp_name_desc(const void *p1, const void *p2) {
    return -strcmp(((yy_chart_item *)p1)->name, ((yy_chart_item *)p2)->name);
}

bool yy_chart_sort_items_with_name(yy_chart *chart, bool ascent) {
    size_t count;
    
    if (!chart) return false;
    count = ARR_COUNT(chart->items, yy_chart_item);
    if (count <= 1) return true;
    qsort(chart->items.hdr, count, sizeof(yy_chart_item),
          ascent ? yy_chart_item_cmp_name_asc : yy_chart_item_cmp_name_desc);
    return true;
}



struct yy_report {
    ARR_TYPE(yy_chart *) charts;
    ARR_TYPE(char *) infos;
};

yy_report *yy_report_new(void) {
    yy_report *report = calloc(1, sizeof(yy_report));
    return report;
}

void yy_report_free(yy_report *report) {
    if (!report) return;
    usize i, count;
    
    count = ARR_COUNT(report->charts, yy_chart);
    for (i = 0; i < count; i++) {
        yy_chart_free(*ARR_GET(report->charts, yy_chart *, i));
    }
    
    count = ARR_COUNT(report->infos, char *);
    for (i = 0; i < count; i++) {
        free(*ARR_GET(report->infos, char *, i));
    }
}

bool yy_report_add_chart(yy_report *report, yy_chart *chart) {
    if (!report || !chart) return false;
    if (ARR_ADD(report->charts, chart, yy_chart *)) {
        chart->ref_count++;
        return true;
    }
    return false;
}

bool yy_report_add_info(yy_report *report, const char *info) {
    if (!report || !info) return false;
    char *str = yy_str_copy(info);
    if (!str) return false;
    if (ARR_ADD(report->infos, str, char *)) {
        return true;
    } else {
        free(str);
        return false;
    }
}

bool yy_report_add_env_info(yy_report *report) {
    char info[1024];
    snprintf(info, sizeof(info), "Compiler: %s", yy_env_get_compiler_desc());
    if (!yy_report_add_info(report, info)) return false;
    snprintf(info, sizeof(info), "OS: %s", yy_env_get_os_desc());
    if (!yy_report_add_info(report, info)) return false;
    snprintf(info, sizeof(info), "CPU: %s", yy_env_get_cpu_desc());
    if (!yy_report_add_info(report, info)) return false;
    snprintf(info, sizeof(info), "CPU Frequency: %.2f MHz", yy_cpu_get_freq() / 1000.0 / 1000.0);
    if (!yy_report_add_info(report, info)) return false;
    return true;
}

bool yy_report_write_html_string(yy_report *report, char **html, usize *len) {
    /* append line string */
#define LS(str) do { \
    if (!yy_sb_append(sb, str)) goto fail; \
    if (!yy_sb_append(sb, "\n")) goto fail; } while(0)
    /* append line format */
#define LF(str, arg) do { \
    if (!yy_sb_printf(sb, str, arg)) goto fail; \
    if (!yy_sb_append(sb, "\n")) goto fail; } while(0)
    /* append string */
#define AS(str) do { if (!yy_sb_append(sb, str)) goto fail; } while(0)
    /* append string format */
#define AF(str, arg) do { if (!yy_sb_printf(sb, str, arg)) goto fail; } while(0)
    /* append string escaped (single quote) */
#define AE(str) do { if (!yy_sb_append_esc(sb, '\'', str)) goto fail; } while(0)
    /* append string escaped (html) */
#define AH(str) do { if (!yy_sb_append_html(sb, str)) goto fail; } while(0)
    /* string with default value */
#define STRDEF(str, def) ((str) ? (str) : (def))
    /* string with bool */
#define STRBOOL(flag) ((flag) ? "true" : "false")
    
    yy_chart_options *op;
    yy_chart_axis_options *x_axis;
    yy_chart_axis_options *y_axis;
    yy_chart_item *item;
    yy_chart_value *val;
    yy_sb _sb;
    yy_sb *sb;
    const char *str;
    const char **str_arr;
    usize i, c, v, val_count, cate_count, max_count;
    
    usize chart_count = ARR_COUNT(report->charts, yy_chart *);
    usize info_count = ARR_COUNT(report->infos, char *);
    
    if (len) *len = 0;
    if (!report || !html) return false;
    if (!yy_sb_init(&_sb, 0)) return false;
    sb = &_sb;
    
    LS("<!DOCTYPE html>");
    LS("<html>");
    LS("<head>");
    LS("<meta charset='utf-8'>");
    LS("<title>Report</title>");
    LS("<script src='https://cdnjs.cloudflare.com/ajax/libs/highcharts/8.2.0/highcharts.min.js'></script>");
    LS("<script src='https://cdnjs.cloudflare.com/ajax/libs/highcharts/8.2.0/modules/series-label.min.js'></script>");
    LS("<script src='https://cdnjs.cloudflare.com/ajax/libs/highcharts/8.2.0/modules/exporting.min.js'></script>");
    LS("<script src='https://cdnjs.cloudflare.com/ajax/libs/highcharts/8.2.0/modules/export-data.min.js'></script>");
    LS("<script src='https://cdnjs.cloudflare.com/ajax/libs/highcharts/8.2.0/modules/offline-exporting.min.js'></script>");
    LS("<script src='https://cdnjs.cloudflare.com/ajax/libs/sortable/0.8.0/js/sortable.min.js'></script>");
    LS("<link rel='stylesheet' href='https://cdnjs.cloudflare.com/ajax/libs/bulma/0.9.0/css/bulma.min.css' />");
    LS("<link rel='stylesheet' href='https://cdnjs.cloudflare.com/ajax/libs/sortable/0.8.0/css/sortable-theme-bootstrap.min.css' />");
    LS("<script>window.onload=Sortable.init</script>");
    LS("<style type='text/css'>");
    LS("hr {");
    LS("    height: 1px;");
    LS("    margin: 5px 0; ");
    LS("    background-color: #999999; ");
    LS("}");
    LS(".table thead {");
    LS("    background-color: rgba(0, 0, 0, 0.05);");
    LS("}");
    LS(".highcharts-data-table table, th, td { ");
    LS("    border: 1px solid gray;");
    LS("    padding: 2pt;");
    LS("}");
    LS(".highcharts-data-table table {");
    LS("    margin: auto;");
    LS("}");
    LS("table .number { ");
    LS("    align-items: initial;");
    LS("    border-radius: initial;");
    LS("    display: table-cell;");
    LS("    font-size: initial;");
    LS("    height: initial;");
    LS("    justify-content: initial;");
    LS("    margin-right: initial;");
    LS("    min-width: initial;");
    LS("    padding: 2pt;");
    LS("    vertical-align: initial;");
    LS("    text-align: initial;");
    LS("    background-color: initial;");
    LS("}");
    LS(".table.is-narrow td, .table.is-narrow th {");
    LS("    padding: .1em .5em;");
    LS("}");
    LS("</style>");
    LS("</head>");
    LS("");
    LS("<body>");
    LS("<nav class='navbar is-light is-fixed-top' role='navigation' aria-label='main navigation'>");
    LS("    <div class='navbar-brand'>");
    LS("        <a class='navbar-item' href='#'>Report</a>");
    LS("        <a role='button' class='navbar-burger burger' aria-label='menu' data-target='main-menu'");
    LS("            onclick='document.querySelector(\".navbar-menu\").classList.toggle(\"is-active\");'>");
    LS("            <span aria-hidden='true'></span>");
    LS("            <span aria-hidden='true'></span>");
    LS("            <span aria-hidden='true'></span>");
    LS("        </a>");
    LS("    </div>");
    LS("    <div id='main-menu' class='navbar-menu'>");
    LS("        <div class='navbar-start'>");
    if (info_count) {
        LS("            <div class='navbar-item has-dropdown is-hoverable'>");
        LS("                <a class='navbar-link'>Info</a>");
        LS("                <div class='navbar-dropdown'>");
        for (c = 0; c < info_count; c++) {
            char *info = *ARR_GET(report->infos, char *, c);
            AS("                    <a class='navbar-item'>"); AH(info); LS("</a>");
        }
        LS("                </div>");
        LS("            </div>");
    }
    LS("            <div class='navbar-item has-dropdown is-hoverable'>");
    LS("                <a class='navbar-link'>Charts</a>");
    LS("                <div class='navbar-dropdown'>");
    for (c = 0; c < chart_count; c++) {
        yy_chart *chart = *ARR_GET(report->charts, yy_chart *, c);
        AF("                    <a class='navbar-item' href='#chart_%d'>", c);
        AH(STRDEF(chart->options.title, "Unnamed Chart"));
        LS("</a>");
    }
    LS("                </div>");
    LS("            </div>");
    LS("        </div>");
    LS("    </div>");
    LS("</nav>");
    
    for (c = 0; c < chart_count; c++) {
        yy_chart *chart = *ARR_GET(report->charts, yy_chart *, c);
        usize item_count = ARR_COUNT(chart->items, yy_chart_item);
        
        op = &chart->options;
        
        // simple html table, not chart
        if (op->type == YY_CHART_TABLE) {
            LS("");
            LF("<a name='chart_%d'></a>", c);
            LS("<div style='width: 60px; height: 60px; margin: 0 auto'></div>");
            AF("<div id='chart_id_%d' style='", c);
            AF("width: %dpx; ", op->width > 0 ? op->width : 800);
            LS("margin: 0 auto' class='table-container'>");
            LS("    <table data-sortable class='table is-bordered is-narrow is-hoverable is-fullwidth sortable-theme-bootstrap'>");
            
            // header
            AS("        <caption>"); AE(STRDEF(op->title, "Unnamed Table")); LS("</caption>");
            LS("        <thead>");
            AS("            <tr>");
            AS("<th>Name</th>");
            if (op->h_axis.categories && op->h_axis.categories[0]) {
                for(str_arr = op->h_axis.categories; str_arr[0]; str_arr++) {
                    AS("<th>"); AE(STRDEF(str_arr[0], "null")); AS("</th>");
                }
            }
            LS("</tr>");
            LS("        </thead>");
            
            // body
            LS("        <tbody>");
            
            for (i = 0; i < item_count; i++) {
                item = ARR_GET(chart->items, yy_chart_item, i);
                AS("            <tr>");
                AS("<td>"); AE(item->name); AS("</td>");
                for ((void)(v = 0), val_count = ARR_COUNT(item->values, yy_chart_value); v < val_count; v++) {
                    val = ARR_GET(item->values, yy_chart_value, v);
                    AS("<td>");
                    if (val->is_null) AS("null");
                    else if (val->is_integer) AF("%d", (int)val->v);
                    else AF("%f", (float)val->v);
                    AS("</td>");
                }
                LS("</tr>");
            }
            
            LS("        </tbody>");
            
            LS("    </table>");
            LS("</div>");
            continue;
        }
        
        
        if (op->type == YY_CHART_BAR) {
            x_axis = &op->v_axis;
            y_axis = &op->h_axis;
        } else {
            x_axis = &op->h_axis;
            y_axis = &op->v_axis;
        }
        
        LS("");
        LF("<a name='chart_%d'></a>", c);
        LS("<div style='width: 60px; height: 60px; margin: 0 auto'></div>");
        AF("<div id='chart_id_%d' style='", c);
        AF("width: %dpx; ", op->width > 0 ? op->width : 800);
        AF("height: %dpx; ", op->height > 0 ? op->height : 500);
        LS("margin: 0 auto'></div>");
        LS("<script type='text/javascript'>");
        LF("Highcharts.chart('chart_id_%d', {", c);
        
        switch (op->type) {
            case YY_CHART_LINE: str = "line"; break;
            case YY_CHART_BAR: str = "bar"; break;
            case YY_CHART_COLUMN: str = "column"; break;
            case YY_CHART_PIE: str = "pie"; break;
            default: str = "line"; break;
        }
        LF("    chart: { type: '%s' },", str);
        AS("    title: { text: '"); AE(STRDEF(op->title, "Unnamed Chart")); LS("' },");
        if (op->subtitle) {
            AS("    subtitle: { text: '"); AE(op->subtitle); LS("' },");
        }
        LS("    credits: { enabled: false },");
        
        /* colors */
        if (op->colors && op->colors[0]) {
            AS("    colors: [");
            for(str_arr = op->colors; str_arr[0]; str_arr++) {
                AS("'"); AE(STRDEF(str_arr[0], "null")); AS("'");
                if (str_arr[1]) AS(", ");
            }
            LS("],");
        }
        
        /* x axis */
        AS("    xAxis: { ");
        if (x_axis->title) {
            AS("title: { text: '"); AE(x_axis->title); AS("' }, ");
        }
        if (x_axis->label_prefix || x_axis->label_suffix) {
            AS("labels: { format: '");
            AE(STRDEF(x_axis->label_prefix, ""));
            AS("{value}");
            AE(STRDEF(x_axis->label_suffix, ""));
            AS("' }, ");
        }
        if (isfinite(x_axis->min)) AF("min: %f, ", x_axis->min);
        if (isfinite(x_axis->max)) AF("max: %f, ", x_axis->max);
        if (isfinite(x_axis->tick_interval)) AF("tickInterval: %f, ", x_axis->tick_interval);
        AF("allowDecimals: %s, ", STRBOOL(x_axis->allow_decimals));
        if (op->type == YY_CHART_LINE && x_axis->categories && x_axis->categories[0]) {
            AS("categories: [");
            for(str_arr = x_axis->categories; str_arr[0]; str_arr++) {
                AS("'"); AE(STRDEF(str_arr[0], "null")); AS("'");
                if (str_arr[1]) AS(", ");
            }
            AS("], ");
        }
        if ((op->type == YY_CHART_BAR || op->type == YY_CHART_COLUMN) && item_count) {
            AS("categories: [");
            for(i = 0; i < item_count; i++) {
                item = ARR_GET(chart->items, yy_chart_item, i);
                AS("'"); AE(STRDEF(item->name, "null")); AS("'");
                if (i + 1 < item_count) AS(", ");
            }
            AS("], ");
        }
        AF("type: '%s' ", x_axis->logarithmic ? "logarithmic" : "linear");
        LS("},");
        
        /* y axis */
        AS("    yAxis: { ");
        if (y_axis->title) {
            AS("title: { text: '"); AE(y_axis->title); AS("' }, ");
        }
        if (y_axis->label_prefix || y_axis->label_suffix) {
            AS("labels: { format: '");
            AE(STRDEF(y_axis->label_prefix, ""));
            AS("{value}");
            AE(STRDEF(y_axis->label_suffix, ""));
            AS("' }, ");
        }
        if (isfinite(y_axis->min)) AF("min: %f, ", y_axis->min);
        if (isfinite(y_axis->max)) AF("max: %f, ", y_axis->max);
        if (isfinite(y_axis->tick_interval)) AF("tickInterval: %f, ", y_axis->tick_interval);
        AF("allowDecimals: %s, ", STRBOOL(y_axis->allow_decimals));
        AF("type: '%s' ", y_axis->logarithmic ? "logarithmic" : "linear");
        LS("},");
                    
        /* tooltip */
        AS("    tooltip: {");
        if (op->tooltip.value_decimals >= 0) {
            AF("valueDecimals: %d, ", op->tooltip.value_decimals);
        }
        if (op->tooltip.value_prefix) {
            AS("valuePrefix: '"); AE(op->tooltip.value_prefix); AS("', ");
        }
        if (op->tooltip.value_suffix) {
            AS("valueSuffix: '"); AE(op->tooltip.value_suffix); AS("', ");
        }
        AF("shared: %s, ", STRBOOL(op->tooltip.shared));
        AF("crosshairs: %s, ", STRBOOL(op->tooltip.crosshairs));
        AS("shadow: false ");
        LS("},");
        
        /* legend */
        AS("    legend: { ");
        switch (op->legend.layout) {
            case YY_CHART_HORIZONTAL: str = "horizontal"; break;
            case YY_CHART_VERTICAL: str = "vertical"; break;
            case YY_CHART_PROXIMATE: str = "proximate"; break;
            default: str = "horizontal"; break;
        }
        AF("layout: '%s', ", str);
        switch (op->legend.h_align) {
            case YY_CHART_LEFT: str = "left"; break;
            case YY_CHART_CENTER: str = "center"; break;
            case YY_CHART_RIGHT: str = "right"; break;
            default: str = "center"; break;
        }
        AF("align: '%s', ", str);
        switch (op->legend.v_align) {
            case YY_CHART_TOP: str = "top"; break;
            case YY_CHART_MIDDLE: str = "middle"; break;
            case YY_CHART_BOTTOM: str = "bottom"; break;
            default: str = "bottom"; break;
        }
        AF("verticalAlign: '%s', ", str);
        AF("enabled: %s ", STRBOOL(op->legend.enabled));
        LS("},");
        
        /* plot */
        LS("    plotOptions: {");
        if (op->type == YY_CHART_LINE) {
            AF("        line: { pointStart: %f, ", op->plot.point_start);
            LF("pointInterval: %f },", op->plot.point_interval);
        }
        if (op->type == YY_CHART_BAR ||
            op->type == YY_CHART_COLUMN) {
            AF("        %s: { ", op->type == YY_CHART_BAR ? "bar" : "column");
            if (isfinite(op->plot.group_padding)) {
                AF("groupPadding: %f, ", op->plot.group_padding);
            }
            if (isfinite(op->plot.point_padding)) {
                AF("pointPadding: %f, ", op->plot.point_padding);
            }
            if (isfinite(op->plot.border_width)) {
                AF("borderWidth: %f, ", op->plot.border_width);
            }
            if (op->plot.group_stacked) {
                AS("stacking: 'normal', ");
            }
            AF("colorByPoint: %s ", STRBOOL(op->plot.color_by_point));
            LS("},");
        }
        AF("        series: { label: { enabled: %s}, ", STRBOOL(op->plot.name_label_enabled));
        AF("dataLabels: { enabled: %s ", STRBOOL(op->plot.value_labels_enabled));
        AS(", allowOverlap: true");
        if (op->plot.value_labels_decimals >= 0) {
            AF(", format: '{point.y:.%df}' ", op->plot.value_labels_decimals);
        }
        LS("} }");
        LS("    },");
        
        /* data */
        LS("    series: [");
        if (op->type == YY_CHART_BAR || op->type == YY_CHART_COLUMN) {
            cate_count = (size_t)yy_chart_axis_category_count(x_axis);
            max_count = 0;
            for (i = 0; i < item_count; i++) {
                item = ARR_GET(chart->items, yy_chart_item, i);
                val_count = ARR_COUNT(item->values, yy_chart_value);
                if (val_count > max_count) max_count = val_count;
            }
            for (v = 0; v < max_count; v++) {
                AS("        { ");
                if (v < cate_count) {
                    AS("name: '"); AE(x_axis->categories[v]); AS("', ");
                }
                AS("data: [");
                for (i = 0; i < item_count; i++) {
                    item = ARR_GET(chart->items, yy_chart_item, i);
                    val_count = ARR_COUNT(item->values, yy_chart_value);
                    val = ARR_GET(item->values, yy_chart_value, v);
                    if (v >= val_count || val->is_null) AS("null");
                    else if (val->is_integer) AF("%d", (int)val->v);
                    else AF("%f", (float)val->v);
                    if (i + 1 < item_count) AS(", ");
                }
                AS("] }"); if (v + 1 < max_count) AS(","); LS("");
            }
        } else {
            for (i = 0; i < item_count; i++) {
                item = ARR_GET(chart->items, yy_chart_item, i);
                AS("        { name: '"); AE(item->name); AS("', data: [");
                for ((void)(v = 0), val_count = ARR_COUNT(item->values, yy_chart_value); v < val_count; v++) {
                    val = ARR_GET(item->values, yy_chart_value, v);
                    if (val->is_null) AS("null");
                    else if (val->is_integer) AF("%d", (int)val->v);
                    else AF("%f", (float)val->v);
                    if (v + 1 < val_count) AS(", ");
                }
                AS("] }"); if (i + 1 < item_count) AS(","); LS("");
            }
        }
        LS("    ]");
        
        LS("});");
        LS("</script>");
    }
    
    LS("");
    LS("</body>");
    AS("</html>");
    
    *html = yy_sb_get_str(sb);
    if (!*html) goto fail;
    if (len) *len = yy_sb_get_len(sb);
    return true;
    
fail:
    yy_sb_release(sb);
    return false;
}

bool yy_report_write_html_file(yy_report *report, const char *path) {
    char *html;
    usize len;
    if (!yy_report_write_html_string(report, &html, &len)) return false;
    bool suc = yy_file_write(path, (u8 *)html, len);
    free(html);
    return suc;
}
