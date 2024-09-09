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

#include "argus/scripting/cabi/error.h"
#include "argus/scripting/error.hpp"

static argus::BindingError &_bind_err_as_ref(argus_binding_error_t err) {
    return *reinterpret_cast<argus::BindingError *>(err);
}

static const argus::BindingError &_bind_err_as_ref(argus_binding_error_const_t err) {
    return *reinterpret_cast<const argus::BindingError *>(err);
}

static argus::ReflectiveArgumentsError &_refl_args_err_as_ref(argus_reflective_args_error_t err) {
    return *reinterpret_cast<argus::ReflectiveArgumentsError *>(err);
}

static const argus::ReflectiveArgumentsError &_refl_args_err_as_ref(argus_reflective_args_error_const_t err) {
    return *reinterpret_cast<const argus::ReflectiveArgumentsError *>(err);
}

extern "C" {

const char *argus_binding_error_get_bound_name(argus_binding_error_const_t err) {
    return _bind_err_as_ref(err).bound_name.c_str();
}

const char *argus_binding_error_get_msg(argus_binding_error_const_t err) {
    return _bind_err_as_ref(err).msg.c_str();
}

void argus_binding_error_free(argus_binding_error_t err) {
    delete &_bind_err_as_ref(err);
}

const char *argus_reflective_args_error_get_reason(argus_reflective_args_error_const_t err) {
    return _refl_args_err_as_ref(err).reason.c_str();
}

void argus_reflective_args_error_free(argus_reflective_args_error_t err) {
    delete &_refl_args_err_as_ref(err);
}

}
