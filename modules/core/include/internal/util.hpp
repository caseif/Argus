#pragma once

// module core
#include "argus/core.hpp"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <vector>

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
