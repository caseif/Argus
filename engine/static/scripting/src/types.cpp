/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2024, Max Roncace <mproncace@protonmail.com>
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

#include "argus/core/engine.hpp"

#include "argus/scripting/types.hpp"
#include "argus/scripting/wrapper.hpp"

#include <functional>
#include <new>
#include <optional>
#include <utility>

#include <cstring>

namespace argus {
    ObjectType::ObjectType(void) = default;

    ObjectType::ObjectType(IntegralType type, size_t size, bool is_const,
            std::optional<std::type_index> type_index,
            std::optional<std::string> type_name,
            std::optional<std::unique_ptr<ScriptCallbackType>> &&callback_type,
            std::optional<ObjectType> primary_type,
            std::optional<ObjectType> secondary_type):
        type(type),
        size(size),
        is_const(is_const),
        type_index(type_index),
        type_name(std::move(type_name)),
        callback_type(std::move(callback_type)),
        primary_type(primary_type.has_value()
                ? std::make_optional<std::unique_ptr<ObjectType>>(
                        std::make_unique<ObjectType>(std::move(primary_type.value())))
                : std::nullopt),
        secondary_type(secondary_type.has_value()
                ? std::make_optional<std::unique_ptr<ObjectType>>(
                        std::make_unique<ObjectType>(std::move(secondary_type.value())))
                : std::nullopt){

    }

    ObjectType::ObjectType(const ObjectType &rhs):
        type(rhs.type),
        size(rhs.size),
        is_const(rhs.is_const),
        type_index(rhs.type_index),
        type_name(rhs.type_name),
        callback_type(rhs.callback_type.has_value()
                ? std::make_optional<std::unique_ptr<ScriptCallbackType>>(
                        std::make_unique<ScriptCallbackType>(*rhs.callback_type.value()))
                : std::nullopt),
        primary_type(rhs.primary_type.has_value()
                ? std::make_optional<std::unique_ptr<ObjectType>>(
                        std::make_unique<ObjectType>(*rhs.primary_type.value()))
                : std::nullopt),
        secondary_type(rhs.secondary_type.has_value()
                ? std::make_optional<std::unique_ptr<ObjectType>>(
                        std::make_unique<ObjectType>(*rhs.secondary_type.value()))
                : std::nullopt) {
    }

    ObjectType::ObjectType(ObjectType &&rhs) noexcept:
        type(rhs.type),
        size(rhs.size),
        is_const(rhs.is_const),
        type_index(rhs.type_index),
        type_name(std::move(rhs.type_name)),
        callback_type(std::move(rhs.callback_type)),
        primary_type(std::move(rhs.primary_type)),
        secondary_type(std::move(rhs.secondary_type)) {
    }

    ObjectType::~ObjectType(void) = default;

    ObjectType &ObjectType::operator=(const ObjectType &rhs) {
        new(this) ObjectType(rhs);
        return *this;
    }

    ObjectType &ObjectType::operator=(ObjectType &&rhs) noexcept = default;

    ObjectWrapper::ObjectWrapper(void):
        type(ObjectType { IntegralType::Void, 0 }),
        value(),
        is_on_heap(false),
        buffer_size(0),
        is_initialized(false) {
    }

    ObjectWrapper::ObjectWrapper(const ObjectType &type, size_t size):
        type(type),
        is_initialized(false) {
        argus_assert(type.type == IntegralType::String || type.type == IntegralType::Pointer
                || type.type == IntegralType::Vector || type.type == IntegralType::VectorRef
                || type.type == IntegralType::Result || type.size == size);

        // override size for pointer type since we're only copying the pointer
        size_t copy_size = type.type == IntegralType::Pointer
                ? sizeof(void *)
                : (type.type == IntegralType::String
                        || type.type == IntegralType::Vector
                        || type.type == IntegralType::VectorRef
                        || type.type == IntegralType::Result)
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

    ObjectWrapper::ObjectWrapper(ObjectWrapper &&rhs) noexcept:
        type(std::move(rhs.type)),
        is_on_heap(rhs.is_on_heap),
        buffer_size(rhs.buffer_size),
        is_initialized(rhs.is_initialized) {

        if (this->is_on_heap) {
            this->heap_ptr = rhs.heap_ptr;
        } else {
            if (rhs.is_initialized) {
                move_wrapped_object(this->type, this->value, rhs.value, this->buffer_size);
            }
        }

        if (rhs.is_on_heap) {
            rhs.heap_ptr = nullptr;
        }
        rhs.buffer_size = 0;
    }

    ObjectWrapper::~ObjectWrapper(void) {
        if (is_initialized && !(this->is_on_heap && this->heap_ptr == nullptr)) {
            destruct_wrapped_object(this->type, this->get_ptr0());
        }

        if (this->is_on_heap && this->heap_ptr != nullptr) {
            free(this->heap_ptr);
        }
    }

    ObjectWrapper &ObjectWrapper::operator=(ObjectWrapper &&rhs) noexcept {
        this->~ObjectWrapper();
        new(this) ObjectWrapper(std::move(rhs));
        return *this;
    }

    void *ObjectWrapper::get_ptr0(void) {
        return is_on_heap ? heap_ptr : value;
    }

    const void *ObjectWrapper::get_ptr0(void) const {
        return is_on_heap ? heap_ptr : value;
    }

    void ObjectWrapper::copy_value_from(const void *src, size_t size) {
        argus_assert(size >= this->buffer_size);
        copy_wrapped_object(this->type, this->get_ptr0(), src, size);
        this->is_initialized = true;
    }

    // this function is only used with struct value types
    void ObjectWrapper::copy_value_into(void *dest, size_t size) const {
        argus_assert(size == this->buffer_size);
        argus_assert(this->is_initialized);
        copy_wrapped_object(this->type, dest, this->get_ptr0(), size);
    }

    ObjectWrapper BoundFieldDef::get_value(ObjectWrapper &wrapper) const {
        return m_access_proxy(wrapper, m_type);
    }

    void BoundFieldDef::set_value(ObjectWrapper &instance, ObjectWrapper &value) const {
        argus_assert(m_assign_proxy.has_value());
        m_assign_proxy.value()(instance, value);
    }

    VectorObject::VectorObject(VectorObjectType type):
        m_obj_type(type) {
    }

    VectorObjectType VectorObject::get_object_type(void) const {
        return m_obj_type;
    }

    ArrayBlob::ArrayBlob(size_t element_size, size_t count, void(*element_dtor)(void *)):
        VectorObject(VectorObjectType::ArrayBlob),
        m_element_size(element_size),
        m_count(count),
        m_element_dtor(element_dtor) {
        if (element_size == 0) {
            crash("Element size must be greater than zero");
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

    DtorProxy ArrayBlob::element_dtor(void) const {
        return m_element_dtor;
    }

    void *ArrayBlob::data(void) {
        return m_blob;
    }

    const void *ArrayBlob::data(void) const {
        return m_blob;
    }

    void *ArrayBlob::operator[](size_t index) {
        if (index >= m_count) {
            crash("ArrayBlob index is out of bounds");
        }

        return reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(m_blob) + (m_element_size * index));
    }

    const void *ArrayBlob::operator[](size_t index) const {
        //return const_cast<const void *>(const_cast<ArrayBlob *>(this)->operator[](index));
        if (index >= m_count) {
            crash("ArrayBlob index is out of bounds");
        }

        return reinterpret_cast<const void *>(reinterpret_cast<uintptr_t>(m_blob) + (m_element_size * index));
    }

    VectorWrapper::VectorWrapper(size_t element_size, ObjectType element_type,
            void *underlying_vec, SizeAccessor get_size_fn, DataAccessor get_data_fn,
            ElementAccessor get_element_fn, ElementMutator set_element_fn):
        VectorObject(VectorObjectType::VectorWrapper),
        m_element_size(element_size),
        m_element_type(std::move(element_type)),
        m_underlying_vec(underlying_vec),
        m_get_size_fn(get_size_fn),
        m_get_data_fn(get_data_fn),
        m_get_element_fn(get_element_fn),
        m_set_element_fn(set_element_fn) {
        if (element_size == 0) {
            crash("Element size must be greater than zero");
        }

        if (underlying_vec == nullptr) {
            crash("Pointer to underlying vector must not be null");
        }

        if (get_size_fn == nullptr) {
            crash("Size accessor for underlying vector must not be null");
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
            crash("Index " + std::to_string(index)
                    + " is out of range in VectorWrapper of size " + std::to_string(size));
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

    ResultWrapper::ResultWrapper(bool is_ok, size_t resolved_size, const ObjectType &resolved_type):
        m_ok(is_ok),
        m_size(resolved_size),
        m_resolved_type(resolved_type) {
    }

    ResultWrapper::ResultWrapper(const ResultWrapper &rhs):
        ResultWrapper(rhs.m_ok, rhs.m_size, rhs.m_resolved_type) {
        copy_wrapped_object(rhs.m_resolved_type, this->m_blob, rhs.m_blob, rhs.m_size);
    }

    ResultWrapper::ResultWrapper(ResultWrapper &&rhs):
        ResultWrapper(rhs.m_ok, rhs.m_size, rhs.m_resolved_type) {
        move_wrapped_object(rhs.m_resolved_type, this->m_blob, rhs.m_blob, rhs.m_size);
    }

    bool ResultWrapper::is_ok(void) const {
        return m_ok;
    }

    size_t ResultWrapper::get_size(void) const {
        return m_size;
    }

    const ObjectType &ResultWrapper::get_value_or_error_type(void) const {
        return m_resolved_type;
    }

    void *ResultWrapper::get_underlying_object_ptr(void) {
        return reinterpret_cast<void *>(m_blob);
    }

    const void *ResultWrapper::get_underlying_object_ptr(void) const {
        return reinterpret_cast<const void *>(m_blob);
    }

    Result<ObjectWrapper, ReflectiveArgumentsError> ResultWrapper::to_object_wrapper(void) const {
        return create_object_wrapper(m_resolved_type, m_blob, m_size);
    }

    void ResultWrapper::copy_value_or_error_from(const void *src) {
        copy_wrapped_object(m_resolved_type, m_blob, src, m_size);
    }
}
