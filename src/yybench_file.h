/*==============================================================================
 * Copyright (C) 2020 YaoYuan <ibireme@gmail.com>.
 * Released under the MIT license (MIT).
 *============================================================================*/

#ifndef yybench_file_h
#define yybench_file_h

#include "yybench_def.h"
#include "yybench_str.h"

#ifdef __cplusplus
extern "C" {
#endif


/*==============================================================================
 * File Utils
 *============================================================================*/

#ifdef _WIN32
#define YY_DIR_SEPARATOR '\\'
#else
#define YY_DIR_SEPARATOR '/'
#endif
#define YY_MAX_PATH 4096

/** Combine multiple component into a path, store the result to the buffer.
    The input parameter list should end with NULL.
    Return false if input is NULL.
    For example: yy_path_combine(buf, "usr", "src", "test.c", NULL); */
#if yy_has_attribute(sentinel)
__attribute__((sentinel))
#endif
bool yy_path_combine(char *buf, const char *path, ...);

/** Remove the last component of path, copy the result to the buffer.
    Return false if input is NULL or no last component. */
bool yy_path_remove_last(char *buf, const char *path);

/** Get the last component of path, copy it to the buffer.
    Return false if input is NULL or no last component. */
bool yy_path_get_last(char *buf, const char *path);

/** Append a file extension to the path, copy the result to the buffer.
    Return false if input is NULL. */
bool yy_path_append_ext(char *buf, const char *path, const char *ext);

/** Remove the file extension, copy the result to the buffer.
    Return false if input is NULL or no extension. */
bool yy_path_remove_ext(char *buf, const char *path);

/** Get the file extension, copy it to the buffer.
    Return false if input is NULL or no extension. */
bool yy_path_get_ext(char *buf, const char *path);


/** Returns whether a path exist. */
bool yy_path_exist(const char *path);

/** Returns whether a path is directory. */
bool yy_path_is_dir(const char *path);


/** Returns content file names of a dir. Returns NULL on error.
    The result should be released by yy_dir_free() */
char **yy_dir_read(const char *path, int *count);

/** Returns content file full paths of a dir. Returns NULL on error.
    The result should be released by yy_dir_free() */
char **yy_dir_read_full(const char *path, int *count);

/** Free the return value of yy_dir_read(). */
void yy_dir_free(char **names);


/** Read a file to memory, dat should be release with free(). */
bool yy_file_read(const char *path, u8 **dat, usize *len);

/** Read a file to memory with zero padding, dat should be release with free(). */
bool yy_file_read_with_padding(const char *path, u8 **dat, usize *len, usize padding);

/** Write data to file, overwrite if exist. */
bool yy_file_write(const char *path, u8 *dat, usize len);

/** Delete a file, returns true if success. */
bool yy_file_delete(const char *path);


/*==============================================================================
 * Data Reader
 *============================================================================*/

/** A data reader */
typedef struct yy_buf yy_dat;

/** Initialize a data reader with file. */
bool yy_dat_init_with_file(yy_dat *dat, const char *path);

/** Initialize a data reader with memory (no copy). */
bool yy_dat_init_with_mem(yy_dat *dat, u8 *mem, usize len);

/** Release the data reader. */
void yy_dat_release(yy_dat *dat);

/** Reset the cursor of data reader. */
void yy_dat_reset(yy_dat *dat);

/** Read a line from data reader (NULL on end or error).
    The cursor will moved to next line.
    The string is not null-terminated. */
char *yy_dat_read_line(yy_dat *dat, usize *len);

/** Read and copy a line from data reader (NULL on end or error).
    The cursor will moved to next line.
    The return value should be release with free(). */
char *yy_dat_copy_line(yy_dat *dat, usize *len);


#ifdef __cplusplus
}
#endif

#endif
