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

#include "argus/scripting/cabi/types.h"

#include "argus/scripting/error.hpp"
#include "argus/scripting/types.hpp"

static const argus::ObjectType &_obj_type_as_ref(argus_object_type_const_t handle) {
    return *reinterpret_cast<const argus::ObjectType *>(handle);
}

static argus::ObjectWrapper &_obj_wrapper_as_ref(argus_object_wrapper_t handle) {
    return *reinterpret_cast<argus::ObjectWrapper *>(handle);
}

static const argus::ObjectWrapper &_obj_wrapper_as_ref(argus_object_wrapper_const_t handle) {
    return *reinterpret_cast<const argus::ObjectWrapper *>(handle);
}

static const argus::ScriptCallbackType &_sc_type_as_ref(argus_script_callback_type_const_t handle) {
    return *reinterpret_cast<const argus::ScriptCallbackType *>(handle);
}

extern "C" {

void argus_object_wrapper_or_refl_args_err_delete(ArgusObjectWrapperOrReflectiveArgsError res) {
    if (res.is_err) {
        delete reinterpret_cast<argus::ReflectiveArgumentsError *>(res.err);
    } else {
        delete reinterpret_cast<argus::ObjectWrapper *>(res.val);
    }
}

argus_object_type_t argus_object_type_new(ArgusIntegralType type, size_t size, bool is_const,
        bool is_refable, const char *type_id, const char *type_name,
        argus_script_callback_type_t script_callback_type, argus_object_type_t primary_type,
        argus_object_type_t secondary_type) {
    UNUSED(is_refable);
    return new argus::ObjectType(
            static_cast<argus::IntegralType>(type),
            size,
            is_const,
            std::make_optional<std::string>(type_id),
            std::make_optional<std::string>(type_name),
            std::make_optional<std::unique_ptr<argus::ScriptCallbackType>>(
                    std::make_unique<argus::ScriptCallbackType>(_sc_type_as_ref(script_callback_type))
            ),
            std::make_optional<argus::ObjectType>(_obj_type_as_ref(primary_type)),
            std::make_optional<argus::ObjectType>(_obj_type_as_ref(secondary_type))
    );
}

void argus_object_type_delete(argus_object_type_t obj_type) {
    delete &_obj_type_as_ref(obj_type);
}

ArgusIntegralType argus_object_type_get_type(argus_object_type_const_t obj_type) {
    return ArgusIntegralType(_obj_type_as_ref(obj_type).type);
}

size_t argus_object_type_get_size(argus_object_type_const_t obj_type) {
    return _obj_type_as_ref(obj_type).size;
}

bool argus_object_type_get_is_const(argus_object_type_const_t obj_type) {
    return _obj_type_as_ref(obj_type).is_const;
}

bool argus_object_type_get_is_refable(argus_object_type_const_t obj_type) {
    return _obj_type_as_ref(obj_type).is_refable;
}

const char *argus_object_type_get_type_id(argus_object_type_const_t obj_type) {
    const auto &type_id = _obj_type_as_ref(obj_type).type_id;
    return type_id.has_value() ? type_id.value().c_str() : nullptr;
}

const char *argus_object_type_get_type_name(argus_object_type_const_t obj_type) {
    const auto &type_name = _obj_type_as_ref(obj_type).type_name;
    return type_name.has_value() ? type_name.value().c_str() : nullptr;
}

argus_script_callback_type_const_t argus_object_type_get_callback_type(argus_object_type_const_t obj_type) {
    const auto &callback_type = _obj_type_as_ref(obj_type).callback_type;
    return callback_type.has_value() ? callback_type.value().get() : nullptr;
}

argus_object_type_const_t argus_object_type_get_primary_type(argus_object_type_const_t obj_type) {
    const auto &prim_type = _obj_type_as_ref(obj_type).primary_type;
    return prim_type.has_value() ? prim_type.value().get() : nullptr;
}

argus_object_type_const_t argus_object_type_get_secondary_type(argus_object_type_const_t obj_type) {
    const auto &sec_type = _obj_type_as_ref(obj_type).secondary_type;
    return sec_type.has_value() ? sec_type.value().get() : nullptr;
}

argus_object_wrapper_t argus_object_wrapper_new(argus_object_type_const_t obj_type, size_t size) {
    return new argus::ObjectWrapper(_obj_type_as_ref(obj_type), size);
}

void argus_object_wrapper_delete(argus_object_wrapper_t obj_wrapper) {
    delete &_obj_wrapper_as_ref(obj_wrapper);
}

argus_object_type_const_t argus_object_wrapper_get_type(argus_object_wrapper_const_t obj_wrapper) {
    return &_obj_wrapper_as_ref(obj_wrapper).type;
}

const void *argus_object_wrapper_get_value(argus_object_wrapper_const_t obj_wrapper) {
    return _obj_wrapper_as_ref(obj_wrapper).get_ptr0();
}

void *argus_object_wrapper_get_value_mut(argus_object_wrapper_t obj_wrapper) {
    return _obj_wrapper_as_ref(obj_wrapper).get_ptr0();
}

size_t argus_object_wrapper_get_buffer_size(argus_object_wrapper_const_t obj_wrapper) {
    return _obj_wrapper_as_ref(obj_wrapper).buffer_size;
}

bool argus_object_wrapper_is_initialized(argus_object_wrapper_const_t obj_wrapper) {
    return _obj_wrapper_as_ref(obj_wrapper).is_initialized;
}

}
