#include "argus/lowlevel.hpp"

#ifdef __WIN32
#include <Windows.h>
#else
#include <time.h>
#ifdef USE_PTHREADS
#include <unistd.h>
#endif
#endif

#define NS_PER_S 1000000000LLU

namespace argus {

    #ifdef __WIN32
    #define WIN32_EPOCH_OFFSET = 11644473600000000ULL

    unsigned long long microtime(void) {
        FILETIME ft;
        GetSystemTimeAsFileTime(&ft);
        unsigned long long tt = ft.dwHighDateTime;
        tt <<= 32;
        tt |= ft.dwLowDateTime;
        tt /= 10;
        tt -= WIN32_EPOCH_OFFSET;
    }
    #else
    unsigned long long microtime(void) {
        timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        return now.tv_sec * 1000000 + now.tv_nsec / 1000;
    }

    #ifdef USE_PTHREADS
    void sleep_nanos(unsigned long long ns) {
        const struct timespec spec = {(long) (ns / NS_PER_S), (long) (ns % NS_PER_S)};
        nanosleep(&spec, NULL);
    }
    #else
    void sleep_nanos(unsigned long long ns) {
        std::this_thread::sleep_for(std::chrono::nanoseconds(ns));
    }
    #endif

    #endif

}
