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

#include "argus/lowlevel/result.hpp"

#include "argus/scripting/cabi/bind.h"
#include "argus/scripting/bind.hpp"

#include <vector>

#include <stddef.h>
#include <stdint.h>

static const argus::ObjectType _obj_type_as_ref(argus_object_type_const_t obj_type) {
    return *reinterpret_cast<const argus::ObjectType *>(obj_type);
}

static std::vector<argus::ObjectType> _unwrap_obj_type_list(size_t count, const argus_object_type_const_t *types) {
    std::vector<argus::ObjectType> vec;
    vec.reserve(count);
    std::transform(types, types + count, vec.begin(), [](const auto &p) { return _obj_type_as_ref(p); });
    return vec;
}

static argus::ObjectWrapper &_obj_wrapper_as_ref(argus_object_wrapper_t wrapper) {
    return *reinterpret_cast<argus::ObjectWrapper *>(wrapper);
}

// ALLOCATES
static ArgusMaybeBindingError _wrap_res(const argus::Result<void, argus::BindingError> &res) {
    if (res.is_err()) {
        return { true, new argus::BindingError(res.unwrap_err()) };
    } else {
        return { false, nullptr };
    }
}

[[maybe_unused]] static argus::Result<void, argus::BindingError> _unwrap_res(const ArgusMaybeBindingError &res) {
    if (res.is_err) {
        return argus::err<void, argus::BindingError>(*reinterpret_cast<argus::BindingError *>(res.error));
    } else {
        return argus::ok<void, argus::BindingError>();
    }
}

static inline argus::Result<argus::ObjectWrapper, argus::ReflectiveArgumentsError> _unwrap_res(
        const ArgusObjectWrapperOrReflectiveArgsError &res) {
    if (res.is_err) {
        auto err_ptr = reinterpret_cast<argus::ReflectiveArgumentsError *>(res.err);
        auto res_cpp = argus::err<argus::ObjectWrapper, argus::ReflectiveArgumentsError>(*err_ptr);
        delete err_ptr;
        return res_cpp;

    } else {
        auto val_ptr = reinterpret_cast<argus::ObjectWrapper *>(res.val);
        auto res_cpp = argus::ok<argus::ObjectWrapper, argus::ReflectiveArgumentsError>(*val_ptr);
        delete val_ptr;
        return res_cpp;
    }
}

static argus::ProxiedNativeFunction _unwrap_proxied_native_fn(ArgusProxiedNativeFunction fn) {
    return [fn](std::vector<argus::ObjectWrapper> &params) {
        std::vector<argus_object_wrapper_t> ptr_vec;
        ptr_vec.reserve(params.size());
        std::transform(params.begin(), params.end(), ptr_vec.begin(), [](auto &p) { return &p; });
        return _unwrap_res(fn(ptr_vec.size(), ptr_vec.data(), nullptr));
    };
}

extern "C" {

ArgusMaybeBindingError argus_bind_type(const char *name, size_t size, const char *type_id, bool is_refable,
        ArgusCopyCtorProxy copy_ctor, ArgusMoveCtorProxy move_ctor, ArgusDtorProxy dtor) {
    auto type_def = argus::BoundTypeDef {
        name,
        size,
        type_id,
        is_refable,
        copy_ctor,
        move_ctor,
        dtor,
        {},
        {},
        {},
        {},
    };
    return _wrap_res(argus::bind_type(type_def));
}

ArgusMaybeBindingError argus_bind_enum(const char *name, size_t width, const char *type_id) {
    auto enum_def = argus::BoundEnumDef {
        name,
        width,
        type_id,
        {},
        {},
    };
    return _wrap_res(argus::bind_enum(enum_def));
}

ArgusMaybeBindingError argus_bind_enum_value(const char *enum_type_id, const char *name, int64_t value) {
    return _wrap_res(argus::bind_enum_value(enum_type_id, name, value));
}

ArgusMaybeBindingError argus_bind_member_field(const char *containing_type_id, const char *name,
        argus_object_type_const_t field_type, ArgusFieldAccessor accessor, const void *accessor_state,
        ArgusFieldMutator mutator, const void *mutator_state) {
    argus::BoundFieldDef def {};
    def.m_name = name;
    def.m_type = _obj_type_as_ref(field_type);
    def.m_access_proxy = [&](const argus::ObjectWrapper &inst, const argus::ObjectType &obj_type) {
        return std::move(_obj_wrapper_as_ref(accessor(&inst, &obj_type, accessor_state)));
    };
    def.m_assign_proxy = mutator != nullptr
            ? std::make_optional([&](argus::ObjectWrapper &inst, argus::ObjectWrapper &val) {
                mutator(&inst, &val, mutator_state);
            })
            : std::nullopt;

    return _wrap_res(argus::bind_member_field(containing_type_id, def));
}

ArgusMaybeBindingError argus_bind_global_function(const char *name, size_t params_count,
        const argus_object_type_const_t *params, argus_object_type_const_t ret_type,
        ArgusProxiedNativeFunction proxied_fn, void *extra) {
    UNUSED(extra);
    auto fn_def = argus::BoundFunctionDef {
        name,
        argus::FunctionType::Global,
        false,
        _unwrap_obj_type_list(params_count, params),
        _obj_type_as_ref(ret_type),
        _unwrap_proxied_native_fn(proxied_fn)
    };
    return _wrap_res(argus::bind_global_function(fn_def));
}

ArgusMaybeBindingError argus_bind_member_static_function(const char *type_id, const char *name, size_t params_count,
        const argus_object_type_const_t *params, argus_object_type_const_t ret_type,
        ArgusProxiedNativeFunction proxied_fn, void *extra) {
    UNUSED(extra);
    auto fn_def = argus::BoundFunctionDef {
            name,
            argus::FunctionType::MemberStatic,
            false,
            _unwrap_obj_type_list(params_count, params),
            _obj_type_as_ref(ret_type),
            _unwrap_proxied_native_fn(proxied_fn),
    };
    return _wrap_res(argus::bind_member_static_function(type_id, fn_def));
}

ArgusMaybeBindingError argus_bind_member_instance_function(const char *type_id, const char *name, bool is_const,
        size_t params_count, const argus_object_type_const_t *params, argus_object_type_const_t ret_type,
        ArgusProxiedNativeFunction proxied_fn, void *extra) {
    UNUSED(extra);
    auto fn_def = argus::BoundFunctionDef {
            name,
            argus::FunctionType::MemberInstance,
            is_const,
            _unwrap_obj_type_list(params_count, params),
            _obj_type_as_ref(ret_type),
            _unwrap_proxied_native_fn(proxied_fn)
    };
    return _wrap_res(argus::bind_member_instance_function(type_id, fn_def));
}

}
