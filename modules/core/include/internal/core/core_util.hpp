/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module core
#include "argus/core.hpp"

#include <algorithm>
#include <atomic>
#include <mutex>
#include <vector>

#include <cstdio>
#include <cstdlib>

namespace argus {

    template <typename ValueType>
    struct IndexedValue {
        Index id;
        ValueType value;

        operator ValueType &() const {
            return value;
        }
    };

    template <typename T, typename CT = const typename std::vector<T>::value_type>
    inline void remove_from_vector(std::vector<T> &vec, CT &item) {
        vec.erase(std::remove(vec.begin(), vec.end(), item));
    }

    template <typename T, typename CT = const typename std::vector<T>::value_type>
    inline void remove_from_vector(std::vector<T> &vec, CT &&item) {
        vec.erase(std::remove(vec.begin(), vec.end(), item));
    }

}
