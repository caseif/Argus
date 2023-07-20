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
#include "argus/scripting/handles.hpp"
#include "argus/lowlevel/logging.hpp"

#include <typeindex>
#include <unordered_map>
#include <utility>

#include <cassert>

namespace argus {
    static std::unordered_map<ScriptVisibleHandle, std::pair<std::type_index, void *>> g_handle_to_ptr_map;
    static std::unordered_map<void *, std::pair<std::type_index, ScriptVisibleHandle>> g_ptr_to_handle_map;

    static ScriptVisibleHandle g_next_handle = 1;

    ScriptVisibleHandle get_or_create_sv_handle(void *ptr, const std::type_index &type) {
        auto handle_it = g_ptr_to_handle_map.find(ptr);
        if (handle_it != g_ptr_to_handle_map.cend()) {
            assert(handle_it->second.first == type);
            return handle_it->second.second;
        } else {
            if (g_next_handle == k_handle_max) {
                // should virtually never happen, even if we assign a billion
                // handles per second it would take around 600 years to get to
                // this point
                Logger::default_logger().fatal("Exhausted script object handles");
            }

            auto handle = g_next_handle++;
            g_handle_to_ptr_map.insert({ handle, { type, ptr } });
            g_ptr_to_handle_map.insert({ ptr, { type, handle } });

            return handle;
        }
    }

    void *deref_sv_handle(ScriptVisibleHandle handle, const std::type_index &expected_type) {
        auto it = g_handle_to_ptr_map.find(handle);
        if (it == g_handle_to_ptr_map.cend()) {
            return nullptr;
        }

        if (it->second.first != expected_type) {
            // either memory corruption or someone is doing something nasty
            return nullptr;
        }

        return it->second.second;
    }

    void invalidate_sv_handle(void *ptr) {
        auto it = g_ptr_to_handle_map.find(ptr);
        if (it == g_ptr_to_handle_map.cend()) {
            return;
        }

        g_handle_to_ptr_map.erase(it->second.second);
        g_ptr_to_handle_map.erase(it);
    }

    ScriptVisible::ScriptVisible(void) = default;

    ScriptVisible::ScriptVisible(const ScriptVisible &) = default;

    ScriptVisible::ScriptVisible(ScriptVisible &&) noexcept = default;

    ScriptVisible::~ScriptVisible(void) {
        invalidate_sv_handle(this);
    }
}
