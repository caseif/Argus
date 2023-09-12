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
        Vector,
        VectorRef
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

    enum class VectorObjectType {
        ArrayBlob,
        VectorWrapper
    };

    class VectorObject {
      private:
        VectorObjectType m_obj_type;

      protected:
        VectorObject(VectorObjectType type);

      public:
        VectorObjectType get_object_type(void);
    };

    // disable non-standard extension warning for zero-sized array member
    #ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable : 4200)
    #endif
    class ArrayBlob : VectorObject {
      private:
        const size_t m_element_size;
        const size_t m_count;
        const std::function<void(void *)> m_element_dtor;
        unsigned char m_blob[0];

      public:
        ArrayBlob(size_t element_size, size_t count, std::function<void(void *)> element_dtor);

        template <typename E>
        ArrayBlob(size_t count) : ArrayBlob(sizeof(E), count, [](void *ptr) { reinterpret_cast<E *>(ptr).~E(); }) {
        }

        ArrayBlob(const ArrayBlob &) = delete;

        ArrayBlob(ArrayBlob &&) = delete;

        ArrayBlob &operator=(const ArrayBlob &) = delete;

        ArrayBlob &operator=(ArrayBlob &&) = delete;

        ~ArrayBlob(void);

        [[nodiscard]] size_t size(void) const;

        [[nodiscard]] size_t element_size(void) const;

        void *data(void);

        void *operator[](size_t index);

        const void *operator[](size_t index) const;

        template <typename T>
        [[nodiscard]] const T &at(size_t index) const {
            if (sizeof(T) != m_element_size) {
                throw std::invalid_argument("Template parameter size does not match element size");
            }
            return *reinterpret_cast<const T *>(this->operator[](index));
        }

        template <typename T>
        [[nodiscard]] T &at(size_t index) {
            return const_cast<T &>(const_cast<const ArrayBlob *>(this)->at<T>(index));
        }

        template <typename T>
        void set(size_t index, T val) {
            *reinterpret_cast<T *>(this->operator[](index)) = val;
        }
    };
    #ifdef _MSC_VER
    #pragma warning(pop)
    #endif

    class VectorWrapper : VectorObject {
       public:
        typedef std::function<size_t(const void *)> size_accessor_t;
        typedef std::function<const void *(void *)> data_accessor_t;
        typedef std::function<void *(void *, size_t)> element_accessor_t;
        typedef std::function<void(void *, size_t, void *)> element_mutator_t;

       private:
        size_t m_element_size;
        ObjectType m_element_type;
        void *m_underlying_vec;
        std::shared_ptr<size_accessor_t> m_get_size_fn;
        std::shared_ptr<data_accessor_t> m_get_data_fn;
        std::shared_ptr<element_accessor_t> m_get_element_fn;
        std::shared_ptr<element_mutator_t> m_set_element_fn;

       public:
        VectorWrapper(size_t element_size, const ObjectType &element_type, void *underlying_vec,
                const size_accessor_t &get_size_fn, const data_accessor_t &get_data_fn,
                const element_accessor_t &get_element_fn, const element_mutator_t &set_element_fn);

        template <typename E>
        VectorWrapper(std::vector<E> &underlying_vec, ObjectType element_type) : VectorWrapper(
                sizeof(E),
                element_type,
                reinterpret_cast<void *>(&underlying_vec),
                [](const void *vec_ptr) { return reinterpret_cast<const std::vector<E> *>(vec_ptr)->size(); },
                [](void *vec_ptr) {
                    return reinterpret_cast<const void *>(reinterpret_cast<const std::vector<E> *>(vec_ptr)->data());
                },
                [](void *vec_ptr, size_t index) { return &reinterpret_cast<std::vector<E> *>(vec_ptr)->at(index); },
                [](void *vec_ptr, size_t index, void *val) {
                    reinterpret_cast<std::vector<E> *>(vec_ptr)->at(index) = *reinterpret_cast<E *>(val);
                }) {
        }

        [[nodiscard]] size_t element_size(void) const;

        [[nodiscard]] const ObjectType &element_type(void) const;

        [[nodiscard]] bool is_const(void) const;

        [[nodiscard]] size_t get_size(void) const;

        [[nodiscard]] const void *get_data(void) const;

        [[nodiscard]] const void *at(size_t index) const;

        template <typename E>
        [[nodiscard]] const E &at(size_t index) const {
            reinterpret_cast<std::vector<E> *>(m_underlying_vec)->at(index);
        }

        [[nodiscard]] void *at(size_t index);

        template <typename E>
        [[nodiscard]] E &at(size_t index) {
            return reinterpret_cast<std::vector<E> *>(m_underlying_vec)->at(index);
        }

        void set(size_t index, void *val);

        template <typename E>
        void set(size_t index, const E &val) {
            if (sizeof(E) != m_element_size) {
                throw std::invalid_argument("Template type size does not match element size of VectorWrapper");
            }
            set(index, reinterpret_cast<void *>(&val));
        }

        template <typename E>
        [[nodiscard]] const std::vector<E> &get_underlying_vector(void) const {
            return *reinterpret_cast<const std::vector<E> *>(m_underlying_vec);
        }

        template <typename E>
        [[nodiscard]] std::vector<E> &get_underlying_vector(void) {
            return *reinterpret_cast<std::vector<E> *>(m_underlying_vec);
        }
    };

    class ScriptBindable {
      public:
        ScriptBindable(void);

        ScriptBindable(const ScriptBindable &);

        ScriptBindable(ScriptBindable &&) noexcept;

        virtual ~ScriptBindable(void) = 0;
    };
}
