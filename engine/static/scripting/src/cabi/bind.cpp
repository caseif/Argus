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
    for (size_t i = 0; i < count; i++) {
        vec.emplace_back(_obj_type_as_ref(types[i]));
    }
    return vec;
}

static argus::ObjectWrapper &_obj_wrapper_as_ref(argus_object_wrapper_t wrapper) {
    return *reinterpret_cast<argus::ObjectWrapper *>(wrapper);
}

static argus::BoundTypeDef &_type_def_as_ref(argus_bound_type_def_t def) {
    return *reinterpret_cast<argus::BoundTypeDef *>(def);
}

static argus::BoundEnumDef &_enum_def_as_ref(argus_bound_enum_def_t def) {
    return *reinterpret_cast<argus::BoundEnumDef *>(def);
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
        auto err = *static_cast<argus::BindingError *>(res.error);
        delete static_cast<argus::BindingError *>(res.error);
        return argus::err<void, argus::BindingError>(err);
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

static argus::ProxiedNativeFunction _unwrap_proxied_native_fn(ArgusProxiedNativeFunction fn, void *extra) {
    return [fn, extra](std::vector<argus::ObjectWrapper> &params) {
        std::vector<argus_object_wrapper_t> ptr_vec;
        ptr_vec.reserve(params.size());
        for (auto &param : params) {
            ptr_vec.push_back(static_cast<void *>(&param));
        }
        return _unwrap_res(fn(ptr_vec.size(), ptr_vec.data(), extra));
    };
}

extern "C" {

argus_bound_type_def_t argus_create_type_def(const char *name, size_t size, const char *type_id, bool is_refable,
        ArgusCopyCtorProxy copy_ctor, ArgusMoveCtorProxy move_ctor, ArgusDtorProxy dtor) {
    auto type_def_res = argus::create_type_def(name, size, type_id, is_refable, copy_ctor, move_ctor, dtor);
    if (type_def_res.is_ok()) {
        return new argus::BoundTypeDef(type_def_res.unwrap());
    } else {
        argus::crash("Failed to create bound type def");
    }
}

argus_bound_enum_def_t argus_create_enum_def(const char *name, size_t width, const char *type_id) {
    auto enum_def_res = argus::create_enum_def(name, width, type_id);
    if (enum_def_res.is_ok()) {
        return new argus::BoundEnumDef(enum_def_res.unwrap());
    } else {
        argus::crash("Failed to create bound enum def");
    }
}

ArgusMaybeBindingError argus_add_enum_value(argus_bound_enum_def_t def, const char *name, int64_t value) {
    return _wrap_res(argus::add_enum_value(_enum_def_as_ref(def), name, value));
}

ArgusMaybeBindingError argus_add_member_field(argus_bound_type_def_t def, const char *name,
        argus_object_type_const_t field_type, ArgusFieldAccessor accessor, const void *accessor_state,
        ArgusFieldMutator mutator, const void *mutator_state) {
    argus::BoundFieldDef field_def {};
    field_def.m_name = name;
    field_def.m_type = _obj_type_as_ref(field_type);
    field_def.m_access_proxy = [accessor, accessor_state](const argus::ObjectWrapper &inst, const argus::ObjectType &obj_type) {
        return std::move(_obj_wrapper_as_ref(accessor(
                reinterpret_cast<const void *>(inst.get_direct_ptr()),
                &obj_type,
                accessor_state
        )));
    };
    field_def.m_assign_proxy = mutator != nullptr
            ? std::make_optional([mutator, mutator_state](argus::ObjectWrapper &inst, argus::ObjectWrapper &val) {
                mutator(
                        reinterpret_cast<void *>(inst.get_direct_ptr()),
                        &val,
                        mutator_state
                );
            })
            : std::nullopt;

    return _wrap_res(argus::add_member_field(_type_def_as_ref(def), field_def));
}

ArgusMaybeBindingError argus_add_member_static_function(argus_bound_type_def_t def, const char *name,
        size_t params_count, const argus_object_type_const_t *params, argus_object_type_const_t ret_type,
        ArgusProxiedNativeFunction proxied_fn, void *extra) {
    UNUSED(extra);
    auto fn_def = argus::BoundFunctionDef {
            name,
            argus::FunctionType::MemberStatic,
            false,
            _unwrap_obj_type_list(params_count, params),
            _obj_type_as_ref(ret_type),
            _unwrap_proxied_native_fn(proxied_fn, extra),
    };
    return _wrap_res(argus::add_member_static_function(_type_def_as_ref(def), fn_def));
}

ArgusMaybeBindingError argus_add_member_instance_function(argus_bound_type_def_t def, const char *name, bool is_const,
        size_t params_count, const argus_object_type_const_t *params, argus_object_type_const_t ret_type,
        ArgusProxiedNativeFunction proxied_fn, void *extra) {
    UNUSED(extra);
    auto fn_def = argus::BoundFunctionDef {
            name,
            argus::FunctionType::MemberInstance,
            is_const,
            _unwrap_obj_type_list(params_count, params),
            _obj_type_as_ref(ret_type),
            _unwrap_proxied_native_fn(proxied_fn, extra)
    };
    return _wrap_res(argus::add_member_instance_function(_type_def_as_ref(def), fn_def));
}

argus_bound_function_def_t argus_create_global_function_def(const char *name, bool is_const, size_t params_count,
        const argus_object_type_const_t *params, argus_object_type_const_t ret_type,
        ArgusProxiedNativeFunction proxied_fn, void *extra) {
    printf("fn_proxy for %s: %p\n", name, reinterpret_cast<const void *>(proxied_fn));
    std::vector<argus::ObjectType> params_vec;
    params_vec.reserve(params_count);
    for (size_t i = 0; i < params_count; i++) {
        params_vec.emplace_back(_obj_type_as_ref(params[i]));
    }
    auto *def = new argus::BoundFunctionDef {
        name,
        argus::FunctionType::Global,
        is_const,
        params_vec,
        _obj_type_as_ref(ret_type),
        _unwrap_proxied_native_fn(proxied_fn, extra),
    };
    return def;
}

void argus_bound_type_def_delete(argus_bound_type_def_t def) {
    delete &_type_def_as_ref(def);
}

void argus_bound_enum_def_delete(argus_bound_enum_def_t def) {
    delete &_enum_def_as_ref(def);
}

void argus_bound_function_def_delete(argus_bound_function_def_t def) {
    delete static_cast<argus::BoundFunctionDef *>(def);
}

}
