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

#include "argus/scripting/cabi/wrapper.h"
#include "argus/scripting/error.hpp"
#include "argus/scripting/manager.hpp"
#include "argus/scripting/wrapper.hpp"

extern "C" {

static inline ArgusObjectWrapperOrReflectiveArgsError _wrap_res(
        argus::Result<argus::ObjectWrapper, argus::ReflectiveArgumentsError> res) {
    if (res.is_ok()) {
        return ArgusObjectWrapperOrReflectiveArgsError {
            false,
            new argus::ObjectWrapper(std::move(res.unwrap())),
            nullptr
        };
    } else {
        const auto &reason = res.unwrap_err().reason;
        auto reason_c = new char[reason.size() + 1];
        memcpy(reason_c, reason.c_str(), reason.size() + 1);
        return ArgusObjectWrapperOrReflectiveArgsError {
                false,
                nullptr,
                reason_c,
        };
    }
}

ArgusObjectWrapperOrReflectiveArgsError argus_create_object_wrapper(argus_object_type_const_t ty, const void *ptr,
        size_t size) {
    return _wrap_res(argus::create_object_wrapper(*reinterpret_cast<const argus::ObjectType *>(ty), ptr, size));
}

void argus_copy_bound_type(const char *type_id, void *dst, const void *src) {
    auto copy_ctor = argus::ScriptManager::instance().get_bound_type_by_type_id(type_id)
        .expect("Tried to copy wrapped object with unbound struct type")
        .copy_ctor;
    argus_assert(copy_ctor != nullptr);
    copy_ctor(dst, src);
}

void argus_move_bound_type(const char *type_id, void *dst, void *src) {
    auto move_ctor = argus::ScriptManager::instance().get_bound_type_by_type_id(type_id)
            .expect("Tried to move wrapped object with unbound struct type")
            .move_ctor;
    argus_assert(move_ctor != nullptr);
    move_ctor(dst, src);
}

}
