/*==============================================================================
 * Copyright (C) 2020 YaoYuan <ibireme@gmail.com>.
 * Released under the MIT license (MIT).
 *============================================================================*/

#ifndef yybench_env_h
#define yybench_env_h

#include "yybench_def.h"

#ifdef __cplusplus
extern "C" {
#endif


/*==============================================================================
 * Runtime Environment
 *============================================================================*/

/** Returns OS description. */
const char *yy_env_get_os_desc(void);

/** Returns CPU description. */
const char *yy_env_get_cpu_desc(void);

/** Returns compiler description. */
const char *yy_env_get_compiler_desc(void);


#ifdef __cplusplus
}
#endif

#endif
