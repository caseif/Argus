/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2023, Max Roncace <mproncace@protonmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <typeindex>
#include <unordered_set>
#include <vector>

#include <cstdint>

namespace argus {
    enum IntegralType {
        Void,
        Integer,
        Float,
        String,
        Struct,
        Pointer,
        Enum
    };

    enum FunctionType {
        Global,
        MemberStatic,
        MemberInstance,
    };

    struct ObjectType {
        IntegralType type;
        size_t size;
        std::optional<std::type_index> type_index = std::nullopt;
        std::optional<std::string> type_name = std::nullopt;
    };

    struct ObjectWrapper {
        ObjectType type;
        union {
            // small values/structs can be stored directly in this struct
            unsigned char value[64];
            // alias for wrappers which wrap pointer or reference types
            void *stored_ptr;
            // larger structs must be allocated on the heap and accessed through this alias
            void *heap_ptr;
        };
        bool is_on_heap;

        void *get_ptr(void) {
            return is_on_heap ? heap_ptr : value;
        }
    };

    typedef std::function<ObjectWrapper(const std::vector<ObjectWrapper> &)> ProxiedFunction;

    struct BoundFunctionDef {
        std::string name;
        FunctionType type;
        std::vector<ObjectType> params;
        ObjectType return_type;
        ProxiedFunction handle;
    };

    struct BoundTypeDef {
        std::string name;
        size_t size;
        std::type_index type_index;
        std::map<std::string, BoundFunctionDef> instance_functions;
        std::map<std::string, BoundFunctionDef> static_functions;
    };

    struct BoundEnumDef {
        std::string name;
        size_t width;
        std::type_index type_index;
        std::map<std::string, uint64_t> values;
        std::unordered_set<uint64_t> all_ordinals;
    };
}
