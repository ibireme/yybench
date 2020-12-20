/*==============================================================================
 * Copyright (C) 2020 YaoYuan <ibireme@gmail.com>.
 * Released under the MIT license (MIT).
 *============================================================================*/

#include "yybench_file.h"


/*==============================================================================
 * File Utils
 *============================================================================*/

bool yy_path_combine(char *buf, const char *path, ...) {
    if (!buf) return false;
    *buf = '\0';
    if (!path) return false;
    
    usize len = strlen(path);
    memmove(buf, path, len);
    const char *hdr = buf;
    buf += len;
    
    va_list args;
    va_start(args, path);
    while (true) {
        const char *item = va_arg(args, const char *);
        if (!item) break;
        if (buf > hdr && *(buf - 1) != YY_DIR_SEPARATOR) {
            *buf++ = YY_DIR_SEPARATOR;
        }
        len = strlen(item);
        if (len && *item == YY_DIR_SEPARATOR) {
            len--;
            item++;
        }
        memmove(buf, item, len);
        buf += len;
    }
    va_end(args);
    
    *buf = '\0';
    return true;
}

bool yy_path_remove_last(char *buf, const char *path) {
    usize len = path ? strlen(path) : 0;
    if (!buf) return false;
    *buf = '\0';
    if (len == 0) return false;
    
    const char *cur = path + len - 1;
    if (*cur == YY_DIR_SEPARATOR) cur--;
    for (; cur >= path; cur--) {
        if (*cur == YY_DIR_SEPARATOR) break;
    }
    len = cur + 1 - path;
    memmove(buf, path, len);
    buf[len] = '\0';
    return len > 0;
}

bool yy_path_get_last(char *buf, const char *path) {
    usize len = path ? strlen(path) : 0;
    const char *end, *cur;
    if (!buf) return false;
    *buf = '\0';
    if (len == 0) return false;
    
    end = path + len - 1;
    if (*end == YY_DIR_SEPARATOR) end--;
    for (cur = end; cur >= path; cur--) {
        if (*cur == YY_DIR_SEPARATOR) break;
    }
    len = end - cur;
    memmove(buf, cur + 1, len);
    buf[len] = '\0';
    return len > 0;
}

bool yy_path_append_ext(char *buf, const char *path, const char *ext) {
    usize len = path ? strlen(path) : 0;
    char tmp[YY_MAX_PATH];
    char *cur = tmp;
    if (!buf) return false;
    
    memcpy(cur, path, len);
    cur += len;
    *cur++ = '.';
    
    len = ext ? strlen(ext) : 0;
    memcpy(cur, ext, len);
    cur += len;
    *cur++ = '\0';
    
    memcpy(buf, tmp, cur - tmp);
    return true;
}

bool yy_path_remove_ext(char *buf, const char *path) {
    if (!buf || !path) return false;
    usize len = strlen(path);
    memmove(buf, path, len + 1);
    for (char *cur = buf + len; cur >= buf; cur--) {
        if (*cur == YY_DIR_SEPARATOR) break;
        if (*cur == '.') {
            *cur = '\0';
            return true;
        }
    }
    return false;
}

bool yy_path_get_ext(char *buf, const char *path) {
    if (!buf || !path) return false;
    usize len = strlen(path);
    for (const char *cur = path + len; cur >= path; cur--) {
        if (*cur == YY_DIR_SEPARATOR) break;
        if (*cur == '.') {
            memmove(buf, cur + 1, len - (cur - path));
            return true;
        }
    }
    *buf = '\0';
    return false;
}



bool yy_path_exist(const char *path) {
    if (!path || !strlen(path)) return false;
#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(path);
    return attrs != INVALID_FILE_ATTRIBUTES;
#else
    struct stat attr;
    if (stat(path, &attr) != 0) return false;
    return true;
#endif
}

bool yy_path_is_dir(const char *path) {
    if (!path || !strlen(path)) return false;
#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(path);
    return (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
    struct stat attr;
    if (stat(path, &attr) != 0) return false;
    return S_ISDIR(attr.st_mode);
#endif
}



static int strcmp_func(void const *a, void const *b) {
    char const *astr = *(char const **)a;
    char const *bstr = *(char const **)b;
    return strcmp(astr, bstr);
}

char **yy_dir_read_opts(const char *path, int *count, bool full) {
#ifdef _WIN32
    struct _finddata_t entry;
    intptr_t handle;
    int idx = 0, alc = 0;
    char **names = NULL, **names_tmp, *search;
    usize path_len = path ? strlen(path) : 0;
    
    if (count) *count = 0;
    if (path_len == 0) return NULL;
    search = malloc(path_len + 3);
    if (!search) return NULL;
    memcpy(search, path, path_len);
    if (search[path_len - 1] == '\\') path_len--;
    memcpy(search + path_len, "\\*\0", 3);
    
    handle = _findfirst(search, &entry);
    if (handle == -1) goto fail;
    
    alc = 4;
    names = malloc(alc * sizeof(char*));
    if (!names) goto fail;
    
    do {
        char *name = (char *)entry.name;
        if (!name || !strlen(name)) continue;
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;
        name = _strdup(name);
        if (!name) goto fail;
        if (idx + 1 >= alc) {
            alc *= 2;
            names_tmp = realloc(names, alc * sizeof(char*));
            if (!names_tmp) goto fail;
            names = names_tmp;
        }
        if (full) {
            char *fullpath = malloc(strlen(path) + strlen(name) + 4);
            if (!fullpath) goto fail;
            yy_path_combine(fullpath, path, name, NULL);
            free(name);
            if (fullpath) name = fullpath;
            else break;
        }
        names[idx] = name;
        idx++;
    } while (_findnext(handle, &entry) == 0);
    _findclose(handle);
    
    if (idx > 1) qsort(names, idx, sizeof(char *), strcmp_func);
    names[idx] = NULL;
    if (count) *count = idx;
    return names;
    
fail:
    if (handle != -1)_findclose(handle);
    if (search) free(search);
    if (names) free(names);
    return NULL;
    
#else
    DIR *dir = NULL;
    struct dirent *entry;
    int idx = 0, alc = 0;
    char **names = NULL, **names_tmp;
    
    if (count) *count = 0;
    if (!path || !strlen(path) || !(dir = opendir(path))) {
        goto fail;
    }
    
    alc = 4;
    names = calloc(1, alc * sizeof(char *));
    if (!names) goto fail;
    
    while ((entry = readdir(dir))) {
        char *name = (char *)entry->d_name;
        if (!name || !strlen(name)) continue;
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;
        if (idx + 1 >= alc) {
            alc *= 2;
            names_tmp = realloc(names, alc * sizeof(char *));
            if (!names_tmp)
                goto fail;
            names = names_tmp;
        }
        name = strdup(name);
        if (!name) goto fail;
        if (full) {
            char *fullpath = malloc(strlen(path) + strlen(name) + 4);
            if (!fullpath) {
                free(name);
                goto fail;
            }
            yy_path_combine(fullpath, path, name, NULL);
            free(name);
            if (fullpath) name = fullpath;
            else break;
        }
        names[idx] = name;
        idx++;
    }
    closedir(dir);
    if (idx > 1) qsort(names, idx, sizeof(char *), strcmp_func);
    names[idx] = NULL;
    if (count) *count = idx;
    return names;
    
fail:
    if (dir) closedir(dir);
    yy_dir_free(names);
    return NULL;
#endif
}

char **yy_dir_read(const char *path, int *count) {
    return yy_dir_read_opts(path, count, false);
}

char **yy_dir_read_full(const char *path, int *count) {
    return yy_dir_read_opts(path, count, true);
}

void yy_dir_free(char **names) {
    if (names) {
        for (int i = 0; ; i++) {
            if (names[i]) free(names[i]);
            else break;
        }
        free(names);
    }
}



bool yy_file_read(const char *path, u8 **dat, usize *len) {
    return yy_file_read_with_padding(path, dat, len, 1); // for string
}

bool yy_file_read_with_padding(const char *path, u8 **dat, usize *len, usize padding) {
    if (!path || !strlen(path)) return false;
    if (!dat || !len) return false;
    
    FILE *file = NULL;
#if _MSC_VER >= 1400
    if (fopen_s(&file, path, "rb") != 0) return false;
#else
    file = fopen(path, "rb");
#endif
    if (file == NULL) return false;
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return false;
    }
    long file_size = ftell(file);
    if (file_size < 0) {
        fclose(file);
        return false;
    }
    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return false;
    }
    void *buf = malloc((usize)file_size + padding);
    if (buf == NULL) {
        fclose(file);
        return false;
    }
    
    if (file_size > 0) {
#if _MSC_VER >= 1400
        if (fread_s(buf, file_size, file_size, 1, file) != 1) {
            free(buf);
            fclose(file);
            return false;
        }
#else
        if (fread(buf, file_size, 1, file) != 1) {
            free(buf);
            fclose(file);
            return false;
        }
#endif
    }
    fclose(file);
    
    memset((char *)buf + file_size, 0, padding);
    *dat = (u8 *)buf;
    *len = (usize)file_size;
    return true;
}

bool yy_file_write(const char *path, u8 *dat, usize len) {
    if (!path || !strlen(path)) return false;
    if (len && !dat) return false;
    
    FILE *file = NULL;
#if _MSC_VER >= 1400
    if (fopen_s(&file, path, "wb") != 0) return false;
#else
    file = fopen(path, "wb");
#endif
    if (file == NULL) return false;
    if (fwrite(dat, len, 1, file) != 1) {
        fclose(file);
        return false;
    }
    if (fclose(file) != 0) {
        file = NULL;
        return false;
    }
    return true;
}

bool yy_file_delete(const char *path) {
    if (!path || !*path) return false;
    return remove(path) == 0;
}



/*==============================================================================
 * Data Reader
 *============================================================================*/

bool yy_dat_init_with_file(yy_dat *dat, const char *path) {
    u8 *mem;
    usize len;
    if (!dat) return false;
    memset(dat, 0, sizeof(yy_dat));
    if (!yy_file_read(path, &mem, &len)) return false;
    dat->hdr = mem;
    dat->cur = mem;
    dat->end = mem + len;
    dat->need_free = true;
    return true;
}

bool yy_dat_init_with_mem(yy_dat *dat, u8 *mem, usize len) {
    if (!dat) return false;
    if (len && !mem) return false;
    dat->hdr = mem;
    dat->cur = mem;
    dat->end = mem + len;
    dat->need_free = false;
    return true;
}

void yy_dat_release(yy_dat *dat) {
    yy_buf_release(dat);
}

void yy_dat_reset(yy_dat *dat) {
    if (dat) dat->cur = dat->hdr;
}
char *yy_dat_read_line(yy_dat *dat, usize *len) {
    if (len) *len = 0;
    if (!dat || dat->cur >= dat->end) return NULL;
    
    u8 *str = dat->cur;
    u8 *cur = dat->cur;
    u8 *end = dat->end;
    while (cur < end && *cur != '\r' && *cur != '\n' && *cur != '\0') cur++;
    if (len) *len = cur - str;
    if (cur < end) {
        if (cur + 1 < end && *cur == '\r' && cur[1] == '\n') cur += 2;
        else if (*cur == '\r' || *cur == '\n' || *cur == '\0') cur++;
    }
    dat->cur = cur;
    return (char *)str;
}

char *yy_dat_copy_line(yy_dat *dat, usize *len) {
    if (len) *len = 0;
    usize _len;
    char *_str = yy_dat_read_line(dat, &_len);
    if (!_str) return NULL;
    char *str = malloc(_len + 1);
    if (!str) return NULL;
    memcpy(str, _str, _len);
    str[_len] = '\0';
    if (len) *len = _len;
    return str;
}

