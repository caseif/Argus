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

#include "argus/core/cabi/engine_config.h"

#include "argus/core/engine_config.hpp"

void set_target_tickrate(unsigned int target_tickrate) {
    argus::set_target_tickrate(target_tickrate);
}

void set_target_framerate(unsigned int target_framerate) {
    argus::set_target_framerate(target_framerate);
}

void set_load_modules(const char *const *module_names, size_t count) {
    std::vector<std::string> vec;
    vec.reserve(count);
    for (size_t i = 0; i < count; i++) {
        vec.emplace_back(module_names[i]);
    }
    argus::set_load_modules(vec);
}

void add_load_module(const char *module_name) {
    argus::add_load_module(module_name);
}

void get_preferred_render_backends(size_t *out_count, const char **out_names) {
    if (out_count == nullptr && out_names == nullptr) {
        return;
    }

    auto &backends = argus::get_preferred_render_backends();

    if (out_count != nullptr) {
        *out_count = backends.size();
    }

    if (out_names != nullptr) {
        for (size_t i = 0; i < backends.size(); i++) {
            out_names[i] = backends[i].c_str();
        }
    }
}

void set_render_backends(const char *const *names, size_t count) {
    std::vector<std::string> vec;
    vec.reserve(count);
    for (size_t i = 0; i < count; i++) {
        vec.emplace_back(names[i]);
    }
    argus::set_render_backends(vec);
}

void add_render_backend(const char *name) {
    argus::add_render_backend(name);
}

void set_render_backend(const char *name) {
    argus::set_render_backend(name);
}

ScreenSpaceScaleMode get_screen_space_scale_mode(void) {
    return ScreenSpaceScaleMode(argus::get_screen_space_scale_mode());
}

void set_screen_space_scale_mode(ScreenSpaceScaleMode mode) {
    argus::set_screen_space_scale_mode(argus::ScreenSpaceScaleMode(mode));
}
