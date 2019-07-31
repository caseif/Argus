#pragma once

#include <cstdio>
#include <cstdlib>

#define _ARGUS_WARN(fmt, ...)   fprintf(stderr, fmt, ##__VA_ARGS__)

#define _ARGUS_FATAL(fmt, ...)  _ARGUS_WARN(fmt, ##__VA_ARGS__); \
                                exit(1)

#define _ARGUS_ASSERT(c, fmt, ...)  if (!(c)) {     \
                                        _ARGUS_FATAL(fmt, ##__VA_ARGS__);   \
                                    }
namespace argus {

    template <typename ValueType>
    struct IndexedValue {
        unsigned long long id;
        ValueType value;
    };

}
