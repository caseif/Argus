#pragma once

// module core
#include "argus/core.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <vector>

#define _ARGUS_WARN(fmt, ...)   fprintf(stderr, fmt, ##__VA_ARGS__)

#define _ARGUS_FATAL(fmt, ...)  _ARGUS_WARN(fmt, ##__VA_ARGS__); \
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
    inline void remove_from_vector(std::vector<T> &vec, T item) {
        vec.erase(std::remove(vec.begin(), vec.end(), item));
    }

}
