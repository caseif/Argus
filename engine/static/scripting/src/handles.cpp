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

#include "argus/lowlevel/misc.hpp"

#include "argus/core/engine.hpp"

#include "argus/core/message.hpp"

#include "argus/scripting/types.hpp"
#include "argus/scripting/handles.hpp"
#include "internal/scripting/handles.hpp"

#include <typeindex>
#include <unordered_map>
#include <utility>

namespace argus {
    static std::unordered_map<ScriptBindableHandle, std::pair<std::string, void *>> g_handle_to_ptr_map;
    static std::unordered_map<void *, std::pair<std::string, ScriptBindableHandle>> g_ptr_to_handle_map;

    static ScriptBindableHandle g_next_handle = 1;

    ScriptBindableHandle get_or_create_sv_handle(void *ptr, const std::string &type_id) {
        auto handle_it = g_ptr_to_handle_map.find(ptr);
        // an object can end up sharing the same address as its first member,
        // so we need to match by both ID _and_ type
        if (handle_it != g_ptr_to_handle_map.cend() && handle_it->second.first == type_id) {
            return handle_it->second.second;
        } else {
            if (g_next_handle == k_handle_max) {
                // should virtually never happen, even if we assign a billion
                // handles per second it would take around 600 years to get to
                // this point
                crash("Exhausted script object handles");
            }

            auto handle = g_next_handle++;
            g_handle_to_ptr_map.insert({ handle, { type_id, ptr }});
            g_ptr_to_handle_map.insert({ ptr, { type_id, handle }});

            return handle;
        }
    }

    void *deref_sv_handle(ScriptBindableHandle handle, const std::string &expected_type_id) {
        auto it = g_handle_to_ptr_map.find(handle);
        if (it == g_handle_to_ptr_map.cend()) {
            return nullptr;
        }

        if (it->second.first != expected_type_id) {
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

    static void _on_object_destroyed(const ObjectDestroyedMessage &message) {
        if (!is_current_thread_update_thread()) {
            return;
        }

        invalidate_sv_handle(message.m_ptr);
    }

    void register_object_destroyed_performer(void) {
        register_message_performer<ObjectDestroyedMessage>(_on_object_destroyed);
    }
}
