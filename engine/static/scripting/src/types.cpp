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

#include "argus/lowlevel/debug.hpp"

#include "argus/scripting/types.hpp"
#include "internal/scripting/handles.hpp"

#include <new>
#include <optional>

#include <cassert>
#include <cstring>

namespace argus {
    ObjectType::ObjectType(void) = default;

    ObjectType::ObjectType(IntegralType type, size_t size, bool is_const,
            std::optional<std::type_index> type_index,
            std::optional<std::string> type_name,
            std::optional<std::unique_ptr<ScriptCallbackType>> &&callback_type,
            std::optional<ObjectType> element_type) :
        type(type),
        size(size),
        is_const(is_const),
        type_index(type_index),
        type_name(std::move(type_name)),
        callback_type(std::move(callback_type)),
        element_type(element_type.has_value()
                ? std::make_optional<std::unique_ptr<ObjectType>>(
                        std::make_unique<ObjectType>(std::move(element_type.value())))
                : std::nullopt) {

    }

    ObjectType::ObjectType(const ObjectType &rhs) :
        type(rhs.type),
        size(rhs.size),
        is_const(rhs.is_const),
        type_index(rhs.type_index),
        type_name(rhs.type_name),
        callback_type(rhs.callback_type.has_value()
                ? std::make_optional<std::unique_ptr<ScriptCallbackType>>(
                        std::make_unique<ScriptCallbackType>(*rhs.callback_type.value()))
                : std::nullopt),
        element_type(rhs.element_type.has_value()
                ? std::make_optional<std::unique_ptr<ObjectType>>(
                        std::make_unique<ObjectType>(*rhs.element_type.value()))
                : std::nullopt) {
    }

    ObjectType::ObjectType(ObjectType &&rhs) noexcept :
        type(rhs.type),
        size(rhs.size),
        is_const(rhs.is_const),
        type_index(rhs.type_index),
        type_name(std::move(rhs.type_name)),
        callback_type(std::move(rhs.callback_type)),
        element_type(std::move(rhs.element_type)) {
    }

    ObjectType::~ObjectType(void) = default;

    ObjectType &ObjectType::operator=(const ObjectType &rhs) {
        type = rhs.type;
        size = rhs.size;
        is_const = rhs.is_const;
        type_index = rhs.type_index;
        type_name = rhs.type_name;
        callback_type = rhs.callback_type.has_value()
                ? std::make_optional<std::unique_ptr<ScriptCallbackType>>(
                        std::make_unique<ScriptCallbackType>(*rhs.callback_type.value()))
                : std::nullopt;
        element_type = rhs.element_type.has_value()
                ? std::make_optional<std::unique_ptr<ObjectType>>(
                        std::make_unique<ObjectType>(*rhs.element_type.value()))
                : std::nullopt;
        return *this;
    }

    ObjectType &ObjectType::operator=(ObjectType &&rhs) noexcept {
        type = rhs.type;
        size = rhs.size;
        is_const = rhs.is_const;
        type_index = rhs.type_index;
        type_name = rhs.type_name;
        callback_type = std::move(rhs.callback_type);
        element_type = std::move(rhs.element_type);
        return *this;
    }

    ObjectWrapper::ObjectWrapper(void) :
            type(ObjectType { IntegralType::Void, 0 }),
            value(),
            is_on_heap(false),
            buffer_size(0),
            copy_ctor(nullptr),
            move_ctor(nullptr),
            dtor(nullptr) {
    }

    ObjectWrapper::ObjectWrapper(const ObjectType &type, size_t size) :
            type(type),
            copy_ctor(nullptr),
            move_ctor(nullptr),
            dtor(nullptr) {
        assert(type.type == IntegralType::String || type.type == IntegralType::Pointer
                || type.type == IntegralType::Vector || type.type == IntegralType::VectorRef || type.size == size);

        // override size for pointer type since we're only copying the pointer
        size_t copy_size = type.type == IntegralType::Pointer
                ? sizeof(void *)
                : type.type == IntegralType::String || type.type == IntegralType::Vector
                        || type.type == IntegralType::VectorRef
                        ? size
                        : type.size;
        this->buffer_size = copy_size;

        if (copy_size <= sizeof(this->value)) {
            // can store directly in ObjectWrapper struct
            this->is_on_heap = false;
        } else {
            // need to alloc on heap
            this->heap_ptr = malloc(copy_size);
            this->is_on_heap = true;
        }
    }

    ObjectWrapper::ObjectWrapper(ObjectWrapper &&rhs) noexcept :
            type(std::move(rhs.type)),
            is_on_heap(rhs.is_on_heap),
            buffer_size(rhs.buffer_size),
            copy_ctor(rhs.copy_ctor),
            move_ctor(rhs.move_ctor),
            // dtor still needs to be able to run on the old object
            dtor(rhs.dtor) {

        if (this->type.type == IntegralType::Struct
                || this->type.type == IntegralType::Callback) {
            assert(this->move_ctor != nullptr);

            void *src_ptr;
            void *dst_ptr;
            if (this->is_on_heap) {
                this->heap_ptr = malloc(this->buffer_size);

                src_ptr = rhs.heap_ptr;
                dst_ptr = this->heap_ptr;
            } else {
                src_ptr = rhs.value;
                dst_ptr = this->value;
            }

            this->move_ctor(dst_ptr, src_ptr);
        } else if (this->is_on_heap) {
            assert(this->move_ctor == nullptr);

            this->is_on_heap = true;
            this->heap_ptr = rhs.heap_ptr;
        } else {
            assert(this->move_ctor == nullptr);
            assert(this->buffer_size < sizeof(this->value));

            memcpy(this->value, rhs.value, this->buffer_size);
        }

        rhs.buffer_size = 0;
        rhs.is_on_heap = false;
        rhs.heap_ptr = nullptr;
        rhs.copy_ctor = nullptr;
        rhs.move_ctor = nullptr;
        rhs.dtor = nullptr;
    }

    ObjectWrapper::~ObjectWrapper(void) {
        if (this->dtor != nullptr) {
            this->dtor(this->get_ptr());
        }

        if (this->is_on_heap) {
            free(this->heap_ptr);
        }
    }

    ObjectWrapper &ObjectWrapper::operator= (ObjectWrapper &&rhs) noexcept {
        this->~ObjectWrapper();
        new(this) ObjectWrapper(std::move(rhs));
        return *this;
    }

    // this function is only used with struct value types
    void ObjectWrapper::copy_value(void *dest, size_t size) const {
        assert(size == this->buffer_size);

        const void *src_ptr;

        if (this->is_on_heap) {
            src_ptr = this->heap_ptr;
        } else {
            assert(this->buffer_size < sizeof(this->value));

            src_ptr = this->value;
        }

        if (this->copy_ctor != nullptr) {
            this->copy_ctor(dest, src_ptr);
        } else {
            memcpy(dest, src_ptr, size);
        }
    }

    ObjectWrapper BoundFieldDef::get_value(ObjectWrapper &wrapper) const {
        return m_access_proxy(wrapper, m_type);
    }

    void BoundFieldDef::set_value(ObjectWrapper &instance, ObjectWrapper &value) const {
        assert(m_assign_proxy.has_value());
        m_assign_proxy.value()(instance, value);
    }

    VectorObject::VectorObject(VectorObjectType type) :
        m_obj_type(type) {
    }

    VectorObjectType VectorObject::get_object_type(void) {
        return m_obj_type;
    }

    ArrayBlob::ArrayBlob(size_t element_size, size_t count, void(*element_dtor)(void *)) :
        VectorObject(VectorObjectType::ArrayBlob),
        m_element_size(element_size),
        m_count(count),
        m_element_dtor(element_dtor) {
        if (element_size == 0) {
            throw std::invalid_argument("Element size must be greater than zero");
        }
    }

    ArrayBlob::~ArrayBlob(void) {
        if (m_element_dtor != nullptr) {
            for (size_t i = 0; i < m_count; i++) {
                m_element_dtor(reinterpret_cast<void *>((*this)[i]));
            }
        }
    }

    size_t ArrayBlob::size(void) const {
        return m_count;
    }

    size_t ArrayBlob::element_size(void) const {
        return m_element_size;
    }

    void *ArrayBlob::data(void) {
        return m_blob;
    }

    void *ArrayBlob::operator[](size_t index) {
        if (index >= m_count) {
            throw std::invalid_argument("Index is out of bounds");
        }

        return reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(m_blob) + (m_element_size * index));
    }

    const void *ArrayBlob::operator[](size_t index) const {
        //return const_cast<const void *>(const_cast<ArrayBlob *>(this)->operator[](index));
        if (index >= m_count) {
            throw std::invalid_argument("Index is out of bounds");
        }

        return reinterpret_cast<const void *>(reinterpret_cast<uintptr_t>(m_blob) + (m_element_size * index));
    }

    VectorWrapper::VectorWrapper(size_t element_size, ObjectType element_type,
            void *underlying_vec, SizeAccessor get_size_fn, DataAccessor get_data_fn,
            ElementAccessor get_element_fn, ElementMutator set_element_fn) :
        VectorObject(VectorObjectType::VectorWrapper),
        m_element_size(element_size),
        m_element_type(std::move(element_type)),
        m_underlying_vec(underlying_vec),
        m_get_size_fn(get_size_fn),
        m_get_data_fn(get_data_fn),
        m_get_element_fn(get_element_fn),
        m_set_element_fn(set_element_fn) {
        if (element_size == 0) {
            throw std::invalid_argument("Element size must be greater than zero");
        }

        if (underlying_vec == nullptr) {
            throw std::invalid_argument("Pointer to underlying vector must not be null");
        }

        if (get_size_fn == nullptr) {
            throw std::invalid_argument("Size accessor for underlying vector must not be null");
        }
    }

    size_t VectorWrapper::element_size(void) const {
        return m_element_size;
    }

    const ObjectType &VectorWrapper::element_type(void) const {
        return m_element_type;
    }

    bool VectorWrapper::is_const(void) const {
        return m_element_type.is_const;
    }

    size_t VectorWrapper::get_size(void) const {
        return (*m_get_size_fn)(m_underlying_vec);
    }

    const void *VectorWrapper::get_data(void) const {
        return (*m_get_data_fn)(m_underlying_vec);
    }

    const void *VectorWrapper::at(size_t index) const {
        auto size = (*m_get_size_fn)(m_underlying_vec);
        if (index >= size) {
            throw std::out_of_range("Index " + std::to_string(index)
                    + " is out of range in vector of size " + std::to_string(size));
        }

        return reinterpret_cast<void *>(reinterpret_cast<uintptr_t>((*m_get_element_fn)(m_underlying_vec, index)));
    }

    void *VectorWrapper::at(size_t index) {
        affirm_precond(!m_element_type.is_const,
                "Cannot get mutable reference to element of const vector via VectorWrapper");

        return const_cast<void *>(const_cast<const VectorWrapper *>(this)->at(index));
    }

    void VectorWrapper::set(size_t index, void *val) {
        affirm_precond(!m_element_type.is_const, "Cannot mutate const vector via VectorWrapper");
        (*m_set_element_fn)(m_underlying_vec, index, val);
    }
}
