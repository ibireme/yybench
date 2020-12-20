/*==============================================================================
 * Copyright (C) 2020 YaoYuan <ibireme@gmail.com>.
 * Released under the MIT license (MIT).
 *============================================================================*/

#include "yybench_str.h"


/*==============================================================================
 * String Utils
 *============================================================================*/

char *yy_str_copy(const char *str) {
    if (!str) return NULL;
    usize len = strlen(str) + 1;
    char *dup = malloc(len);
    if (dup) memcpy(dup, str, len);
    return dup;
}

bool yy_str_contains(const char *str, const char *search) {
    if (!str || !search) return false;
    return strstr(str, search) != NULL;
}

bool yy_str_has_prefix(const char *str, const char *prefix) {
    if (!str || !prefix) return false;
    usize len1 = strlen(str);
    usize len2 = strlen(prefix);
    if (len2 > len1) return false;
    return memcmp(str, prefix, len2) == 0;
}

bool yy_str_has_suffix(const char *str, const char *suffix) {
    if (!str || !suffix) return false;
    usize len1 = strlen(str);
    usize len2 = strlen(suffix);
    if (len2 > len1) return false;
    return memcmp(str + (len1 - len2), suffix, len2) == 0;
}



/*==============================================================================
 * Memory Buffer
 *============================================================================*/

bool yy_buf_init(yy_buf *buf, usize len) {
    if (!buf) return false;
    if (len < 16) len = 16;
    memset(buf, 0, sizeof(yy_buf));
    buf->hdr = malloc(len);
    if (!buf->hdr) return false;
    buf->cur = buf->hdr;
    buf->end = buf->hdr + len;
    buf->need_free = true;
    return true;
}

void yy_buf_release(yy_buf *buf) {
    if (!buf || !buf->hdr) return;
    if (buf->need_free) free(buf->hdr);
    memset(buf, 0, sizeof(yy_buf));
}

usize yy_buf_len(yy_buf *buf) {
    if (!buf) return 0;
    return buf->cur - buf->hdr;
}

bool yy_buf_grow(yy_buf *buf, usize len) {
    if (!buf) return false;
    if ((usize)(buf->end - buf->cur) >= len) return true;
    if (!buf->hdr) return yy_buf_init(buf, len);
    
    usize use = buf->cur - buf->hdr;
    usize alc = buf->end - buf->hdr;
    do {
        if (alc * 2 < alc) return false; /* overflow */
        alc *= 2;
    } while(alc - use < len);
    u8 *tmp = (u8 *)realloc(buf->hdr, alc);
    if (!tmp) return false;
    
    buf->cur = tmp + (buf->cur - buf->hdr);
    buf->hdr = tmp;
    buf->end = tmp + alc;
    return true;
}

bool yy_buf_append(yy_buf *buf, u8 *dat, usize len) {
    if (!buf) return false;
    if (len == 0) return true;
    if (!dat) return false;
    if (!yy_buf_grow(buf, len)) return false;
    memcpy(buf->cur, dat, len);
    buf->cur += len;
    return true;
}



/*==============================================================================
 * String Builder
 *============================================================================*/

bool yy_sb_init(yy_sb *sb, usize len) {
    return yy_buf_init(sb, len);
}

void yy_sb_release(yy_sb *sb) {
    yy_buf_release(sb);
}

usize yy_sb_get_len(yy_sb *sb) {
    if (!sb) return 0;
    return sb->cur - sb->hdr;
}

char *yy_sb_get_str(yy_sb *sb) {
    if (!sb || !sb->hdr) return NULL;
    if (sb->cur >= sb->end) {
        if (!yy_buf_grow(sb, 1)) return NULL;
    }
    *sb->cur = '\0';
    return (char *)sb->hdr;
}

char *yy_sb_copy_str(yy_sb *sb, usize *len) {
    if (!sb || !sb->hdr) return NULL;
    usize sb_len = sb->cur - sb->hdr;
    char *str = (char *)malloc(sb_len + 1);
    if (!str) return NULL;
    memcpy(str, sb->hdr, sb_len);
    str[sb_len] = '\0';
    if (len) *len = sb_len;
    return str;
}

bool yy_sb_append(yy_sb *sb, const char *str) {
    if (!str) return false;
    usize len = strlen(str);
    if (!yy_buf_grow(sb, len + 1)) return false;
    memcpy(sb->cur, str, len);
    sb->cur += len;
    return true;
}

bool yy_sb_append_html(yy_sb *sb, const char *str) {
    if (!sb || !str) return false;
    const char *cur = str;
    usize hdr_pos = sb->cur - sb->hdr;
    while (true) {
        usize esc_len;
        const char *esc;
        if (*cur == '\0') break;
        switch (*cur) {
            case '"': esc = "&quot;"; esc_len = 6; break;
            case '&': esc = "&amp;"; esc_len = 5; break;
            case '\'': esc = "&#39;"; esc_len = 5; break;
            case '<': esc = "&lt;"; esc_len = 4; break;
            case '>': esc = "&gt;"; esc_len = 4; break;
            default: esc = NULL; esc_len = 0; break;
        }
        if (esc_len) {
            usize len = cur - str;
            if (!yy_buf_grow(sb, len + esc_len + 1)) {
                sb->cur = sb->hdr + hdr_pos;
                return false;
            }
            memcpy(sb->cur, str, len);
            sb->cur += len;
            memcpy(sb->cur, esc, esc_len);
            sb->cur += esc_len;
            str = cur + 1;
        }
        cur++;
    }
    if (cur != str) {
        if (!yy_sb_append(sb, str)) {
            sb->cur = sb->hdr + hdr_pos;
            return false;
        }
    }
    return true;
}

bool yy_sb_append_esc(yy_sb *sb, char esc, const char *str) {
    if (!sb || !str) return false;
    const char *cur = str;
    usize hdr_pos = sb->cur - sb->hdr;
    while (true) {
        char c = *cur;
        if (c == '\0') break;
        if (c == '\\') {
            if (*(cur++) == '\0') break;
            else continue;
        }
        if (c == esc) {
            usize len = cur - str + 2;
            if (!yy_buf_grow(sb, len + 1)) {
                sb->cur = sb->hdr + hdr_pos;
                return false;
            }
            memcpy(sb->cur, str, len - 2);
            sb->cur[len - 2] = '\\';
            sb->cur[len - 1] = esc;
            sb->cur += len;
            str = cur + 1;
        }
        cur++;
    }
    if (cur != str) {
        if (!yy_sb_append(sb, str)) {
            sb->cur = sb->hdr + hdr_pos;
            return false;
        }
    }
    return true;
}

bool yy_sb_printf(yy_sb *sb, const char *fmt, ...) {
    if (!sb || !fmt) return false;
    usize incr_size = 64;
    int len = 0;
    do {
        if (!yy_buf_grow(sb, incr_size + 1)) return false;
        va_list args;
        va_start(args, fmt);
        len = vsnprintf((char *)sb->cur, incr_size, fmt, args);
        va_end(args);
        if (len < 0) return false; /* error */
        if ((usize)len < incr_size) break; /* success */
        if (incr_size * 2 < incr_size) return false; /* overflow */
        incr_size *= 2;
    } while (true);
    sb->cur += len;
    return true;
}


