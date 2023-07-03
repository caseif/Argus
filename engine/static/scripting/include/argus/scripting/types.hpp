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
#include <string>
#include <vector>

namespace argus {
    struct ObjectProxy {
        union {
            // small values/structs can be stored directly in this struct
            unsigned char value[64];
            // alias for proxies wrapping pointer or reference types
            void *stored_ptr;
            // larger structs must be allocated on the heap and accessed through this alias
            void *heap_ptr;
        };
        bool is_on_heap;
    };

    typedef std::function<ObjectProxy(const std::vector<ObjectProxy> &)> ProxiedFunction;

    enum IntegralType {
        Integer,
        Float,
        String,
        Opaque
    };

    struct ObjectType {
        IntegralType type;
        size_t size;
        std::string type_name;
    };

    struct BoundFunctionDef {
        std::string name;
        std::vector<ObjectType> params;
        ObjectType return_type;
        ProxiedFunction handle;
    };

    struct BoundTypeDef {
        std::string name;
        size_t size;
        //std::vector<BoundMemberDef> members;
        std::vector<BoundFunctionDef> instance_functions;
        std::vector<BoundFunctionDef> static_functions;
    };

    /*struct BoundMemberDef {
        std::string name;
        std::string type;
    };*/
}
