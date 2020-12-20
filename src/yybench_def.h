/*==============================================================================
 * Copyright (C) 2020 YaoYuan <ibireme@gmail.com>.
 * Released under the MIT license (MIT).
 *============================================================================*/

#ifndef yybench_def_h
#define yybench_def_h


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>


#ifdef _WIN32
#   include <windows.h>
#   include <io.h>
#   include <intrin.h>
#else
#   ifdef __APPLE__
#       include <TargetConditionals.h>
#       include <mach/mach_init.h>
#       include <mach/mach_time.h>
#       include <mach/thread_policy.h>
#       include <mach/thread_act.h>
#       include <sys/types.h>
#       include <sys/sysctl.h>
#       include <sys/utsname.h>
#   else
#       ifndef _GNU_SOURCE
#           define _GNU_SOURCE
#       endif
#       ifndef __USE_GNU
#           define __USE_GNU
#       endif
#   endif
#   include <sys/time.h>
#   include <pthread.h>
#   include <sched.h>
#   include <dirent.h>
#   include <sys/stat.h>
#endif

/* architecture */
#if defined(__x86_64) || defined(__x86_64__) || \
    defined(__amd64) || defined(__amd64__) || \
    defined(_M_AMD64) || defined(_M_X64)
#   define YY_ARCH_X64 1
#   define YY_ARCH_64  1
#elif defined(i386) || defined(__i386) || defined(__i386__) || \
    defined(_X86_) || defined(__X86__) || defined(_M_IX86) || \
    defined(__I86__) || defined(__IA32__) || defined(__THW_INTEL)
#   define YY_ARCH_X86 1
#   define YY_ARCH_32  1
#elif defined(__arm64) || defined(__arm64__) || \
    defined(__aarch64__) || defined(_M_ARM64)
#   define YY_ARCH_ARM64 1
#   define YY_ARCH_64    1
#elif defined(__arm) || defined(__arm__) || defined(_ARM_) || \
    defined(_ARM) || defined(_M_ARM) || defined(__TARGET_ARCH_ARM)
#   define YY_ARCH_ARM32 1
#   define YY_ARCH_32    1
#endif

#if !YY_ARCH_64 && YY_ARCH_32
#   if defined(_LP64) || defined(__LP64__) || defined(__64BIT__)
#       define YY_ARCH_64 1
#   endif
#endif

/* compiler builtin check (clang) */
#ifndef yy_has_builtin
#   ifdef __has_builtin
#       define yy_has_builtin(x) __has_builtin(x)
#   else
#       define yy_has_builtin(x) 0
#   endif
#endif

/* compiler attribute check (gcc/clang) */
#ifndef yy_has_attribute
#   ifdef __has_attribute
#       define yy_has_attribute(x) __has_attribute(x)
#   else
#       define yy_has_attribute(x) 0
#   endif
#endif

/* include check (gcc/clang) */
#ifndef yy_has_include
#   ifdef __has_include
#       define yy_has_include(x) __has_include(x)
#   else
#       define yy_has_include(x) 0
#   endif
#endif

/* inline */
#ifndef yy_inline
#   if _MSC_VER >= 1200
#       define yy_inline __forceinline
#   elif defined(_MSC_VER)
#       define yy_inline __inline
#   elif yy_has_attribute(always_inline) || __GNUC__ >= 4
#       define yy_inline __inline__ __attribute__((always_inline))
#   elif defined(__clang__) || defined(__GNUC__)
#       define yy_inline __inline__
#   elif defined(__cplusplus) || (__STDC__ >= 1 && __STDC_VERSION__ >= 199901L)
#       define yy_inline inline
#   else
#       define yy_inline
#   endif
#endif

/* noinline */
#ifndef yy_noinline
#   if _MSC_VER >= 1200
#       define yy_noinline __declspec(noinline)
#   elif yy_has_attribute(noinline) || __GNUC__ >= 4
#       define yy_noinline __attribute__((noinline))
#   else
#       define yy_noinline
#   endif
#endif

/* likely */
#ifndef yy_likely
#   if yy_has_builtin(__builtin_expect) || __GNUC__ >= 4
#       define yy_likely(expr) __builtin_expect(!!(expr), 1)
#   else
#       define yy_likely(expr) (expr)
#   endif
#endif

/* unlikely */
#ifndef yy_unlikely
#   if yy_has_builtin(__builtin_expect) || __GNUC__ >= 4
#       define yy_unlikely(expr) __builtin_expect(!!(expr), 0)
#   else
#       define yy_unlikely(expr) (expr)
#   endif
#endif

/* stdint */
#if YY_HAS_STDINT_H || yy_has_include(<stdint.h>) || \
    _MSC_VER >= 1600 || (__STDC__ >= 1 && __STDC_VERSION__ >= 199901L) || \
    defined(_STDINT_H) || defined(_STDINT_H_) || defined(__CLANG_STDINT_H) || \
    defined(_STDINT_H_INCLUDED)
#   include <stdint.h>
#elif defined(_MSC_VER)
#   if _MSC_VER < 1300
typedef signed char         int8_t;
typedef signed short        int16_t;
typedef signed int          int32_t;
typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned int        uint32_t;
typedef signed __int64      int64_t;
typedef unsigned __int64    uint64_t;
#   else
typedef signed __int8       int8_t;
typedef signed __int16      int16_t;
typedef signed __int32      int32_t;
typedef unsigned __int8     uint8_t;
typedef unsigned __int16    uint16_t;
typedef unsigned __int32    uint32_t;
typedef signed __int64      int64_t;
typedef unsigned __int64    uint64_t;
#   endif
#endif

/* stdbool */
#if YY_HAS_STDBOOL_H || yy_has_include(<stdbool.h>) || \
    _MSC_VER >= 1800 || (__STDC__ >= 1 && __STDC_VERSION__ >= 199901L)
#   include <stdbool.h>
#elif !defined(__bool_true_false_are_defined)
#   define __bool_true_false_are_defined 1
#   if defined(__cplusplus)
#       if defined(__GNUC__) && !defined(__STRICT_ANSI__)
#           define _Bool bool
#           if __cplusplus < 201103L
#               define bool bool
#               define false false
#               define true true
#           endif
#       endif
#   else
#       define bool unsigned char
#       define true 1
#       define false 0
#   endif
#endif

/* assert */
#define yy_assert(expr) \
    if (!(expr)) { \
        fprintf(stderr, "Assertion failed: %s (%s: %d)\n", #expr, __FILE__, __LINE__); \
        abort(); \
    };

#define yy_assertf(expr, ...) \
    if (!(expr)) { \
        fprintf(stderr, "Assertion failed: %s (%s: %d)\n", #expr, __FILE__, __LINE__); \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n"); \
        abort(); \
    };

/* test */
#if yy_has_include("yy_xctest.h")
#   include "yy_xctest.h"
#   define yy_test_case(name) \
        void name(void)
#else
#   define yy_test_case(name) \
    void name(void); \
    int main(int argc, const char * argv[]) { \
        name(); \
        return 0; \
    } \
    void name(void)
#endif

/* Type define for primitive types. */
typedef float       f32;
typedef double      f64;
typedef int8_t      i8;
typedef uint8_t     u8;
typedef int16_t     i16;
typedef uint16_t    u16;
typedef int32_t     i32;
typedef uint32_t    u32;
typedef int64_t     i64;
typedef uint64_t    u64;
typedef size_t      usize;

#endif
