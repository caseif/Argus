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

#include "argus/lowlevel/logging.hpp"

#include "argus/render/common/backend.hpp"
#include "internal/render/common/backend.hpp"

#include <map>
#include <optional>
#include <stdexcept>
#include <string>

namespace argus {
    static std::map<std::string, ActivateRenderBackendFn> g_render_backend_activate_fns;
    static std::string g_active_render_backend;

    void register_render_backend(const std::string &id, ActivateRenderBackendFn activate_fn) {
        if (g_render_backend_activate_fns.find(id) != g_render_backend_activate_fns.end()) {
            throw std::invalid_argument("Render backend is already registered for provided ID");
        }
        g_render_backend_activate_fns[id] = activate_fn;
        Logger::default_logger().debug("Successfully registered render backend with ID %s", id.c_str());
    }

    std::optional<ActivateRenderBackendFn> get_render_backend_activate_fn(std::string backend_id) {
        auto it = g_render_backend_activate_fns.find(backend_id);
        if (it != g_render_backend_activate_fns.end()) {
            return it->second;
        } else {
            return nullptr;
        }
    }

    void unregister_backend_activate_fns(void) {
        g_render_backend_activate_fns.clear();
    }

    const std::string &get_active_render_backend(void) {
        return g_active_render_backend;
    }

    void set_active_render_backend(const std::string &backend) {
        g_active_render_backend = backend;
    }
}
