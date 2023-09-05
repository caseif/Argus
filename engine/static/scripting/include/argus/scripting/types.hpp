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
#include <memory>
#include <optional>
#include <string>
#include <typeindex>
#include <unordered_set>
#include <vector>

#include <cassert>
#include <cstdint>
#include <stdexcept>

namespace argus {
    enum IntegralType {
        Void,
        Integer,
        Float,
        Boolean,
        String,
        Struct,
        Pointer,
        Enum,
        Callback,
        Type,
        Vector
    };

    enum FunctionType {
        Global,
        MemberStatic,
        MemberInstance,
    };

    struct ScriptCallbackType;

    struct ObjectType {
        IntegralType type;
        size_t size;
        bool is_const = false;
        std::optional<std::type_index> type_index = std::nullopt;
        std::optional<std::string> type_name = std::nullopt;
        std::optional<std::unique_ptr<ScriptCallbackType>> callback_type = std::nullopt;
        std::optional<std::unique_ptr<ObjectType>> element_type = std::nullopt;

        ObjectType(void);

        ObjectType(
                IntegralType type,
                size_t size,
                bool is_const = false,
                std::optional<std::type_index> type_index = std::nullopt,
                std::optional<std::string> type_name = std::nullopt,
                std::optional<std::unique_ptr<ScriptCallbackType>> &&callback_type = std::nullopt,
                std::optional<ObjectType> element_type = std::nullopt
        );

        ObjectType(const ObjectType &rhs);

        ObjectType(ObjectType &&rhs) noexcept;

        ~ObjectType(void);

        ObjectType &operator=(const ObjectType &rhs);

        ObjectType &operator=(ObjectType &&rhs) noexcept;
    };

    struct ScriptCallbackType {
        std::vector<ObjectType> params;
        ObjectType return_type;
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
        size_t buffer_size;
        std::optional<std::function<void(void *, const void *)>> copy_ctor;
        std::optional<std::function<void(void *, void *)>> move_ctor;
        std::optional<std::function<void(void *)>> dtor;

        ObjectWrapper(void);

        ObjectWrapper(const ObjectType &type, size_t size);

        // copying must be explicit, generally this type should only be moved
        ObjectWrapper(const ObjectWrapper &rhs) = delete;

        ObjectWrapper(ObjectWrapper &&rhs) noexcept;

        ~ObjectWrapper(void);

        ObjectWrapper &operator= (const ObjectWrapper &rhs) = delete;

        ObjectWrapper &operator= (ObjectWrapper &&rhs) noexcept;

        void copy_value(void *dest, size_t size) const;

        void *get_ptr(void) {
            return is_on_heap ? heap_ptr : value;
        }
    };

    typedef std::function<ObjectWrapper(const std::vector<ObjectWrapper> &)> ProxiedFunction;

    struct BoundFunctionDef {
        std::string name;
        FunctionType type;
        bool is_const;
        std::vector<ObjectType> params;
        ObjectType return_type;
        ProxiedFunction handle;
    };

    struct BoundFieldDef {
        std::string m_name;
        ObjectType m_type;
        std::function<ObjectWrapper(const ObjectWrapper &, const ObjectType &)> m_get_const_proxy;
        std::function<ObjectWrapper(ObjectWrapper &, const ObjectType &)> m_get_mut_proxy;
        std::optional<std::function<void (ObjectWrapper &, ObjectWrapper &)>> m_assign_proxy;

        ObjectWrapper get_const_proxy(const ObjectWrapper &wrapper) {
            return m_get_const_proxy(wrapper, m_type);
        }

        ObjectWrapper get_mut_proxy(ObjectWrapper &wrapper) {
            return m_get_mut_proxy(wrapper, m_type);
        }
    };

    struct BoundTypeDef {
        std::string name;
        size_t size;
        std::type_index type_index;
        // the copy and move ctors and dtor are only used for struct value and callback types
        std::optional<std::function<void(void *dst, const void *src)>> copy_ctor;
        std::optional<std::function<void(void *dst, void *src)>> move_ctor;
        std::optional<std::function<void(void *obj)>> dtor;
        std::map<std::string, BoundFunctionDef> instance_functions;
        std::map<std::string, BoundFunctionDef> static_functions;
        std::map<std::string, BoundFieldDef> fields;
    };

    struct BoundEnumDef {
        std::string name;
        size_t width;
        std::type_index type_index;
        std::map<std::string, uint64_t> values;
        std::unordered_set<uint64_t> all_ordinals;
    };

    struct free_deleter_t {
        void operator()(void *ptr) { free(ptr); }
    };

    // disable non-standard extension warning for zero-sized array member
    #ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable : 4200)
    #endif
    class ArrayBlob {
      private:
        size_t m_element_size;
        size_t m_count;
        unsigned char m_blob[0];

      public:
        ArrayBlob(size_t element_size, size_t count);

        template <typename T>
        ArrayBlob(size_t count) : ArrayBlob(sizeof(T), count) {
        }

        void *operator[](size_t index);

        const void *operator[](size_t index) const;

        template <typename T>
        T &operator[](size_t index) {
            if (sizeof(T) != m_element_size) {
                throw std::invalid_argument("Template parameter size does not match element size");
            }
            return *reinterpret_cast<T *>(this->operator[](index));
        }

        template <typename T>
        const T &operator[](size_t index) const {
            return const_cast<const T &>(const_cast<ArrayBlob *>(this)->operator[]<T>(index));
        }
    };
    #ifdef _MSC_VER
    #pragma warning(pop)
    #endif

    class ScriptBindable {
      public:
        ScriptBindable(void);

        ScriptBindable(const ScriptBindable &);

        ScriptBindable(ScriptBindable &&) noexcept;

        virtual ~ScriptBindable(void) = 0;
    };
}
