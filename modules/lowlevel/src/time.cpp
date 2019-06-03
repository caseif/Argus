#include "argus/argus_lowlevel.hpp"

#ifdef __WIN32
#include <Windows.h>
#else
#include <time.h>
#endif

#ifdef __WIN32
#define WIN32_EPOCH_OFFSET = 11644473600000000ULL

unsigned long long microtime() {
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    unsigned long long tt = ft.dwHighDateTime;
    tt <<= 32;
    tt |= ft.dwLowDateTime;
    tt /= 10;
    tt -= WIN32_EPOCH_OFFSET;
}
#else
unsigned long long microtime() {
    timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now.tv_sec * 1000000 + now.tv_nsec / 1000;
}
#endif
