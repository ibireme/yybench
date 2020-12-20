/*==============================================================================
 * Copyright (C) 2020 YaoYuan <ibireme@gmail.com>.
 * Released under the MIT license (MIT).
 *============================================================================*/

#ifndef yybench_rand_h
#define yybench_rand_h

#include "yybench_def.h"

#ifdef __cplusplus
extern "C" {
#endif


/*==============================================================================
 * Random Number Generator
 *============================================================================*/

/** Reset the random number generator with default seed. */
void yy_random_reset(void);

/** Generate a uniformly distributed 32-bit random number. */
u32 yy_random32(void);

/** Generate a uniformly distributed number, where 0 <= r < bound. */
u32 yy_random32_uniform(u32 bound);

/** Generate a uniformly distributed number, where min <= r <= max. */
u32 yy_random32_range(u32 min, u32 max);

/** Generate a uniformly distributed 64-bit random number. */
u64 yy_random64(void);

/** Generate a uniformly distributed number, where 0 <= r < bound. */
u64 yy_random64_uniform(u64 bound);

/** Generate a uniformly distributed number, where min <= r <= max. */
u64 yy_random64_range(u64 min, u64 max);


#ifdef __cplusplus
}
#endif

#endif
