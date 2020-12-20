/*==============================================================================
 * Copyright (C) 2020 YaoYuan <ibireme@gmail.com>.
 * Released under the MIT license (MIT).
 *============================================================================*/

#include "yybench_rand.h"


/*==============================================================================
 * Random Number Generator
 * PCG random: http://www.pcg-random.org
 * A fixed seed should be used to ensure repeatability of the test or benchmark.
 *============================================================================*/

#define YY_RANDOM_STATE_INIT (((u64)0x853C49E6U << 32) + 0x748FEA9BU)
#define YY_RANDOM_INC_INIT (((u64)0xDA3E39CBU << 32) + 0x94B95BDBU)
#define YY_RANDOM_MUL (((u64)0x5851F42DU << 32) + 0x4C957F2DU)

static u64 yy_random_state = YY_RANDOM_STATE_INIT;
static u64 yy_random_inc = YY_RANDOM_INC_INIT;

void yy_random_reset(void) {
    yy_random_state = YY_RANDOM_STATE_INIT;
    yy_random_inc = YY_RANDOM_INC_INIT;
}

u32 yy_random32(void) {
    u32 xorshifted, rot;
    u64 oldstate = yy_random_state;
    yy_random_state = oldstate * YY_RANDOM_MUL + yy_random_inc;
    xorshifted = (u32)(((oldstate >> 18) ^ oldstate) >> 27);
    rot = (u32)(oldstate >> 59);
    return (xorshifted >> rot) | (xorshifted << (((u32)-(i32)rot) & 31));
}

u32 yy_random32_uniform(u32 bound) {
    u32 r, threshold = (u32)(-(i32)bound) % bound;
    if (bound < 2) return 0;
    while (true) {
        r = yy_random32();
        if (r >= threshold) return r % bound;
    }
}

u32 yy_random32_range(u32 min, u32 max) {
    return yy_random32_uniform(max - min + 1) + min;
}

u64 yy_random64(void) {
    return (u64)yy_random32() << 32 | yy_random32();
}

u64 yy_random64_uniform(u64 bound) {
    u64 r, threshold = ((u64)-(i64)bound) % bound;
    if (bound < 2) return 0;
    while (true) {
        r = yy_random64();
        if (r >= threshold) return r % bound;
    }
}

u64 yy_random64_range(u64 min, u64 max) {
    return yy_random64_uniform(max - min + 1) + min;
}


