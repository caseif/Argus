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

#include "argus/scripting/types.hpp"

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
            buffer_size(0) {
    }

    ObjectWrapper::ObjectWrapper(const ObjectType &type, size_t size) :
            type(type) {
        assert(type.type == IntegralType::String || type.type == IntegralType::Pointer
                || type.size == size);

        // override size for pointer type since we're only copying the pointer
        size_t copy_size = type.type == IntegralType::Pointer
                ? sizeof(void *)
                : type.type == IntegralType::String
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
            copy_ctor(std::move(rhs.copy_ctor)),
            move_ctor(std::move(rhs.move_ctor)),
            // dtor still needs to be able to run on the old object
            dtor(rhs.dtor) {

        if (this->type.type == IntegralType::Struct
                || this->type.type == IntegralType::Callback) {
            assert(this->move_ctor.has_value());

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

            this->move_ctor.value()(dst_ptr, src_ptr);
        } else if (this->is_on_heap) {
            assert(!this->move_ctor.has_value());

            this->is_on_heap = true;
            this->heap_ptr = rhs.heap_ptr;
        } else {
            assert(!this->move_ctor.has_value());
            assert(this->buffer_size < sizeof(this->value));

            memcpy(this->value, rhs.value, this->buffer_size);
        }

        rhs.buffer_size = 0;
        rhs.is_on_heap = false;
        rhs.heap_ptr = nullptr;
        rhs.copy_ctor = std::nullopt;
        rhs.move_ctor = std::nullopt;
        rhs.dtor = std::nullopt;
    }

    ObjectWrapper::~ObjectWrapper(void) {
        if (this->dtor.has_value()) {
            this->dtor.value()(this->get_ptr());
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

        if (this->copy_ctor.has_value()) {
            this->copy_ctor.value()(dest, src_ptr);
        } else {
            memcpy(dest, src_ptr, size);
        }
    }

    ArrayBlob::ArrayBlob(size_t element_size, size_t count) {
        if (element_size == 0) {
            throw std::invalid_argument("Element size must be greater than zero");
        }
        m_element_size = element_size;
        m_count = count;
    }

    void *ArrayBlob::operator[](size_t index) {
        if (index >= m_count) {
            throw std::invalid_argument("Index is out of bounds");
        }

        return reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(m_blob) + (m_element_size * index));
    }

    const void *ArrayBlob::operator[](size_t index) const {
        return const_cast<const void *>(const_cast<ArrayBlob *>(this)->operator[](index));
    }
}
