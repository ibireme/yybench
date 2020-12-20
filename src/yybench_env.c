/*==============================================================================
 * Copyright (C) 2020 YaoYuan <ibireme@gmail.com>.
 * Released under the MIT license (MIT).
 *============================================================================*/

#include "yybench_env.h"

const char *yy_env_get_os_desc(void) {
#if defined(__MINGW64__)
    return "Windows (MinGW-w64)";
#elif defined(__MINGW32__)
    return "Windows (MinGW)";
#elif defined(__CYGWIN__) && defined(ARCH_64_DEFINED)
    return "Windows (Cygwin x64)";
#elif defined(__CYGWIN__)
    return "Windows (Cygwin x86)";
#elif defined(_WIN64)
    return "Windows 64-bit";
#elif defined(_WIN32)
    return "Windows 32-bit";
    
#elif defined(__APPLE__)
#   if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
#       if defined(YY_ARCH_64)
    return "iOS 64-bit";
#       else
    return "iOS 32-bit";
#       endif
#   elif defined(TARGET_OS_OSX) && TARGET_OS_OSX
#       if defined(YY_ARCH_64)
    return "macOS 64-bit";
#       else
    return "macOS 32-bit";
#       endif
#   else
#       if defined(YY_ARCH_64)
    return "Apple OS 64-bit";
#       else
    return "Apple OS 32-bit";
#       endif
#   endif
    
#elif defined(__ANDROID__)
#   if defined(YY_ARCH_64)
    return "Android 64-bit";
#   else
    return "Android 32-bit";
#   endif
    
#elif defined(__linux__) || defined(__linux) || defined(__gnu_linux__)
#   if defined(YY_ARCH_64)
    return "Linux 64-bit";
#   else
    return "Linux 32-bit";
#   endif
    
#elif defined(__BSD__) || defined(__FreeBSD__)
#   if defined(YY_ARCH_64)
    return "BSD 64-bit";
#   else
    return "BSD 32-bit";
#   endif
    
#else
#   if defined(YY_ARCH_64)
    return "Unknown OS 64-bit";
#   else
    return "Unknown OS 32-bit";
#   endif
#endif
}

const char *yy_env_get_cpu_desc(void) {
#if defined(__APPLE__)
    static char brand[256] = {0};
    size_t size = sizeof(brand);
    static bool finished = false;
    struct utsname sysinfo;
    
    if (finished) return brand;
    /* works for macOS */
    if (sysctlbyname("machdep.cpu.brand_string", (void *)brand, &size, NULL, 0) == 0) {
        if (strlen(brand) > 0) finished = true;
    }
    /* works for iOS, returns device model such as "iPhone9,1 ARM64_T8010" */
    if (!finished) {
        uname(&sysinfo);
        const char *model = sysinfo.machine;
        const char *cpu = sysinfo.version;
        if (cpu) {
            cpu = strstr(cpu, "RELEASE_");
            if (cpu) cpu += 8;
        }
        if (model || cpu) {
            snprintf(brand, sizeof(brand), "%s %s", model ? model : "", cpu ? cpu : "");
            finished = true;
        }
    }
    if (!finished) {
        snprintf(brand, sizeof(brand), "Unknown CPU");
        finished = true;
    }
    return brand;
    
#elif defined(_WIN32)
#if defined(__x86_64__) || defined(__amd64__) || \
defined(_M_IX86) || defined(_M_AMD64)
    static char brand[0x40] = { 0 };
    static bool finished = false;
    int cpui[4] = { 0 };
    int nexids, i;
    
    if (finished) return brand;
    __cpuid(cpui, 0x80000000);
    nexids = cpui[0];
    if (nexids >= 0x80000004) {
        for (i = 2; i <= 4; i++) {
            memset(cpui, 0, sizeof(cpui));
            __cpuidex(cpui, i + 0x80000000, 0);
            memcpy(brand + (i - 2) * sizeof(cpui), cpui, sizeof(cpui));
        }
        finished = true;
    }
    if (!finished || strlen(brand) == 0) {
        snprintf(brand, sizeof(brand), "Unknown CPU");
        finished = true;
    }
    return brand;
#   else
    return "Unknown CPU";
#   endif
    
#else
#   define BUF_LENGTH 1024
    static char buf[BUF_LENGTH], *res = NULL;
    static bool finished = false;
    const char *prefixes[] = {
        "model name", /* x86 */
        "CPU part",   /* arm */
        "cpu model\t",/* mips */
        "cpu\t"       /* powerpc */
    };
    int i, len;
    FILE *fp;
    
    if (res) return res;
    fp = fopen("/proc/cpuinfo", "r");
    if (fp) {
        while (!res) {
            memset(buf, 0, BUF_LENGTH);
            if (fgets(buf, BUF_LENGTH - 1, fp) == NULL) break;
            for (i = 0; i < (int)(sizeof(prefixes) / sizeof(char *)) && !res; i++) {
                if (strncmp(prefixes[i], buf, strlen(prefixes[i])) == 0) {
                    res = buf + strlen(prefixes[i]);
                }
            }
        }
        fclose(fp);
    }
    if (res) {
        while (*res == ' ' || *res == '\t' || *res == ':') res++;
        for (i = 0, len = (int)strlen(res); i < len; i++) {
            if (res[i] == '\t') res[i] = ' ';
            if (res[i] == '\r' || res[i] == '\n') res[i] = '\0';
        }
    } else {
        res = "Unknown CPU";
    }
    finished = true;
    return res;
#endif
}

const char *yy_env_get_compiler_desc(void) {
    static char buf[512] = {0};
    static bool finished = false;
    if (finished) return buf;
    
#if defined(__ICL) || defined(__ICC) || defined(__INTEL_COMPILER)
    int v, r; /* version, revision */
#   if defined(__INTEL_COMPILER)
    v = __INTEL_COMPILER;
#   elif defined(__ICC)
    v = __ICC;
#   else
    v = __ICL;
#   endif
    r = (v - (v / 100) * 100) / 10;
    v = v / 100;
    snprintf(buf, sizeof(buf), "Intel C++ Compiler %d.%d", v, r);
    
#elif defined(__ARMCC_VERSION)
    int v, r;
    v = __ARMCC_VERSION; /* PVVbbbb or Mmmuuxx */
    r = (v - (v / 1000000) * 1000000) / 1000;
    v = v / 1000000;
    snprintf(buf, sizeof(buf), "ARM Compiler %d.%d", v, r);
    
#elif defined(_MSC_VER)
    /* https://docs.microsoft.com/en-us/cpp/preprocessor/predefined-macros */
    const char *vc;
#   if _MSC_VER >= 1930
    vc = "";
#   elif _MSC_VER >= 1920
    vc = " 2019";
#   elif _MSC_VER >= 1910
    vc = " 2017";
#   elif _MSC_VER >= 1900
    vc = " 2015";
#   elif _MSC_VER >= 1800
    vc = " 2013";
#   elif _MSC_VER >= 1700
    vc = " 2012";
#   elif _MSC_VER >= 1600
    vc = " 2010";
#   elif _MSC_VER >= 1500
    vc = " 2008";
#   elif _MSC_VER >= 1400
    vc = " 2005";
#   elif _MSC_VER >= 1310
    vc = " 7.1";
#   elif _MSC_VER >= 1300
    vc = " 7.0";
#   elif _MSC_VER >= 1200
    vc = " 6.0";
#   else
    vc = "";
#   endif
    snprintf(buf, sizeof(buf), "Microsoft Visual C++%s (%d)", vc, _MSC_VER);
    
#elif defined(__clang__)
#   if defined(__apple_build_version__)
    /* Apple versions: https://en.wikipedia.org/wiki/Xcode#Latest_versions */
    snprintf(buf, sizeof(buf), "Clang %d.%d.%d (Apple version)",
             __clang_major__, __clang_minor__, __clang_patchlevel__);
#   else
    snprintf(buf, sizeof(buf), "Clang %d.%d.%d",
             __clang_major__, __clang_minor__, __clang_patchlevel__);
#   endif
    
#elif defined(__GNUC__)
    const char *ext;
#   if defined(__CYGWIN__)
    ext = " (Cygwin)";
#   elif defined(__MINGW64__)
    ext = " (MinGW-w64)";
#   elif defined(__MINGW32__)
    ext = " (MinGW)";
#   else
    ext = "";
#   endif
#   if defined(__GNUC_PATCHLEVEL__)
    snprintf(buf, sizeof(buf), "GCC %d.%d.%d%s", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__, ext);
#   else
    snprintf(buf, sizeof(buf), "GCC %d.%d%s", __GNUC__, __GNUC_MINOR__, ext);
#   endif
    
#else
    snprintf(buf, sizeof(buf), "Unknown Compiler");
#endif
    
    finished = true;
    return buf;
}
