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

#include "internal/render_opengl_legacy/gl_util.hpp"
#include "internal/render_opengl_legacy/renderer/material_mgmt.hpp"
#include "internal/render_opengl_legacy/renderer/shader_mgmt.hpp"
#include "internal/render_opengl_legacy/renderer/texture_mgmt.hpp"
#include "internal/render_opengl_legacy/state/render_bucket.hpp"
#include "internal/render_opengl_legacy/state/renderer_state.hpp"
#include "internal/render_opengl_legacy/state/scene_state.hpp"

#include <vector>

namespace argus {
    void deinit_material(RendererState &state, const std::string &material) {
        Logger::default_logger().debug("De-initializing material %s", material.c_str());
        for (auto *scene_state : state.all_scene_states) {
            std::vector<BucketKey> to_remove;

            for (auto &[bucket_key, _] : scene_state->render_buckets) {
                if (bucket_key.material_uid == material) {
                    to_remove.push_back(bucket_key);
                }
            }

            for (auto &key : to_remove) {
                auto bucket = scene_state->render_buckets.find(key)->second;
                try_delete_buffer(bucket->vertex_buffer);
                bucket->~RenderBucket();

                scene_state->render_buckets.erase(key);
            }
        }

        auto &programs = state.linked_programs;
        auto program_it = programs.find(material);
        if (program_it != programs.end()) {
            deinit_program(program_it->second.handle);
            programs.erase(program_it);
        }

        if (auto texture_uid = state.material_textures.find(material);
                texture_uid != state.material_textures.end()) {
            release_texture(state, texture_uid->second);
        }
    }
}
