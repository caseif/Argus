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

#pragma once

#include <typeindex>

#include <cstdint>

namespace argus {
    typedef uint64_t ScriptBindableHandle;

    constexpr const ScriptBindableHandle k_null_handle = 0;
    constexpr const ScriptBindableHandle k_handle_max = UINT64_MAX;

    [[nodiscard]] ScriptBindableHandle get_or_create_sv_handle(const std::string &type_id, void *ptr);

    template<typename T>
    [[nodiscard]] ScriptBindableHandle get_or_create_sv_handle(T &obj) {
        return get_or_create_handle(typeid(obj), &obj);
    }

    [[nodiscard]] void *deref_sv_handle(ScriptBindableHandle handle, const std::string &expected_type_id);

    template<typename T>
    [[nodiscard]] T *deref_handle(ScriptBindableHandle handle) {
        return deref_sv_handle(handle, typeid(T).name());
    }
}
