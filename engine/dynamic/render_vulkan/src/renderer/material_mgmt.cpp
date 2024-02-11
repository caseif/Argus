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

#include "internal/render_vulkan/renderer/material_mgmt.hpp"
#include "internal/render_vulkan/state/renderer_state.hpp"
#include "internal/render_vulkan/util/pipeline.hpp"
#include "internal/render_vulkan/util/texture.hpp"

#include <string>

namespace argus {
    void deinit_material(RendererState &state, const std::string &material_uid) {
        auto pipeline_it = state.material_pipelines.find(material_uid);
        if (pipeline_it != state.material_pipelines.cend()) {
            destroy_pipeline(state.device, pipeline_it->second);
        }

        auto texture_uid_it = state.material_textures.find(material_uid);
        if (texture_uid_it != state.material_textures.cend()) {
            auto texture_it = state.prepared_textures.find(texture_uid_it->second);
            auto new_rc = texture_it->second.release();
            if (new_rc == 0) {
                destroy_texture(state.device, texture_it->second);
            }
        }
    }
}
