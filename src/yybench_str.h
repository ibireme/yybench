/*==============================================================================
 * Copyright (C) 2020 YaoYuan <ibireme@gmail.com>.
 * Released under the MIT license (MIT).
 *============================================================================*/

#ifndef yybench_str_h
#define yybench_str_h

#include "yybench_def.h"

#ifdef __cplusplus
extern "C" {
#endif


/*==============================================================================
 * String Utils
 *============================================================================*/

/** Copy a string, same as strdup(). */
char *yy_str_copy(const char *str);

/** Returns whether the string contains a given string. */
bool yy_str_contains(const char *str, const char *search);

/** Returns whether the string begins with a prefix. */
bool yy_str_has_prefix(const char *str, const char *prefix);

/** Returns whether the string ends with a suffix. */
bool yy_str_has_suffix(const char *str, const char *suffix);


/*==============================================================================
 * Memory Buffer
 *============================================================================*/

/** A memory buffer s*/
typedef struct yy_buf {
    u8 *cur; /* cursor between hdr and end */
    u8 *hdr; /* head of the buffer */
    u8 *end; /* tail of the buffer */
    bool need_free;
} yy_buf;

/** Initialize a memory buffer with length. */
bool yy_buf_init(yy_buf *buf, usize len);

/** Release the memory in buffer. */
void yy_buf_release(yy_buf *buf);

/** Returns the used length of buffer (cur - hdr). */
usize yy_buf_len(yy_buf *buf);

/** Increase memory buffer and let (end - cur >= len). */
bool yy_buf_grow(yy_buf *buf, usize len);

/** Append data to buffer and move cursor. */
bool yy_buf_append(yy_buf *buf, u8 *dat, usize len);


/*==============================================================================
 * String Builder
 *============================================================================*/

/** A string builder */
typedef struct yy_buf yy_sb;

/** Initialize a string builder with capacity. */
bool yy_sb_init(yy_sb *buf, usize len);

/** Release the string builder. */
void yy_sb_release(yy_sb *buf);

/** Returns the length of string. */
usize yy_sb_get_len(yy_sb *sb);

/** Returns the inner string */
char *yy_sb_get_str(yy_sb *sb);

/** Copies and returns the string, should be released with free(). */
char *yy_sb_copy_str(yy_sb *sb, usize *len);

/** Append string. */
bool yy_sb_append(yy_sb *sb, const char *str);

/** Append string and escape html. */
bool yy_sb_append_html(yy_sb *sb, const char *str);

/** Append string and escape single character. */
bool yy_sb_append_esc(yy_sb *sb, char esc, const char *str);

/** Append string with format. */
bool yy_sb_printf(yy_sb *sb, const char *fmt, ...);


#ifdef __cplusplus
}
#endif

#endif
