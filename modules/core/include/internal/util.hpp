#pragma once

// module core
#include "argus/core.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <vector>

#define STRINGIZE_DETAIL(x) #x
#define STRINGIZE(x) STRINGIZE_DETAIL(x)

#ifdef _ARGUS_DEBUG_MODE
#define _GENERIC_PRINT(stream, level, system, fmt, ...) fprintf(stream, "[%s][%s] " __FILE__ ":" STRINGIZE(__LINE__) ": " fmt, level, system, ##__VA_ARGS__)
#else
#define _GENERIC_PRINT(stream, level, system, fmt, ...) fprintf(stream, "[%s][%s] " fmt, level, system, ##__VA_ARGS__)
#endif

#define _ARGUS_PRINT(stream, level, fmt, ...) _GENERIC_PRINT(stream, level, "Argus", fmt, ##__VA_ARGS__)

#ifdef _ARGUS_DEBUG_MODE
#define _ARGUS_DEBUG(fmt, ...) _ARGUS_PRINT(stdout, "DEBUG", fmt, ##__VA_ARGS__)
#else
#define _ARGUS_DEBUG(fmt, ...)
#endif

#define _ARGUS_INFO(fmt, ...) _ARGUS_PRINT(stdout, "INFO", fmt, ##__VA_ARGS__)
#define _ARGUS_WARN(fmt, ...) _ARGUS_PRINT(stderr, "WARN", fmt, ##__VA_ARGS__)

#define _ARGUS_FATAL(fmt, ...)  _ARGUS_PRINT(stderr, "FATAL", fmt, ##__VA_ARGS__); \
                                exit(1)

#define _ARGUS_ASSERT(c, fmt, ...)  if (!(c)) {     \
                                        _ARGUS_FATAL(fmt, ##__VA_ARGS__);   \
                                    }
namespace argus {

    template <typename ValueType>
    struct IndexedValue {
        Index id;
        ValueType value;
    };

    template<typename T>
    inline void remove_from_vector(std::vector<T> &vec, const T item) {
        vec.erase(std::remove(vec.begin(), vec.end(), item));
    }

}
