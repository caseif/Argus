/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2022, Max Roncace <mproncace@protonmail.com>
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

#include "argus/resman/resource.hpp"
#include "argus/resman/resource_manager.hpp"

#include "argus/render/common/material.hpp"
#include "argus/render/common/texture_data.hpp"

#include "internal/render_opengl/types.hpp"
#include "internal/render_opengl/renderer/texture_mgmt.hpp"
#include "internal/render_opengl/state/renderer_state.hpp"

#include "aglet/aglet.h"

#include <map>
#include <string>
#include <utility>

#include <cstddef>
#include <cstdint>

namespace argus {
    void get_or_load_texture(RendererState &state, const Resource &material_res) {
        auto &texture_uid = material_res.get<Material>().get_texture_uid();

        auto existing_it = state.prepared_textures.find(texture_uid);
        if (existing_it != state.prepared_textures.end()) {
            existing_it->second.acquire();
            state.material_textures.insert({ material_res.uid, texture_uid });
            return;
        }

        auto &texture_res = ResourceManager::instance().get_resource(texture_uid);
        auto &texture = texture_res.get<TextureData>();
        
        _ARGUS_ASSERT(texture.width <= INT32_MAX, "Texture width is too big");
        _ARGUS_ASSERT(texture.height <= INT32_MAX, "Texture height is too big");

        texture_handle_t handle;

        if (AGLET_GL_ARB_direct_state_access) {
            glCreateTextures(GL_TEXTURE_2D, 1, &handle);

            glTextureParameteri(handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTextureParameteri(handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTextureParameteri(handle, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTextureParameteri(handle, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        } else {
            glGenTextures(1, &handle);
            glBindTexture(GL_TEXTURE_2D, handle);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        }

        //glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

        if (AGLET_GL_ARB_direct_state_access) {
            glTextureStorage2D(handle, 1, GL_RGBA8, texture.width, texture.height);
        } else if (AGLET_GL_ARB_texture_storage) {
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, texture.width, texture.height);
        } else {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture.width, texture.height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                    nullptr);
        }

        for (uint32_t y = 0; y < texture.height; y++) {
            if (AGLET_GL_ARB_direct_state_access) {
                glTextureSubImage2D(handle, 0, 0, static_cast<int32_t>(y), texture.width, 1, GL_RGBA,
                        GL_UNSIGNED_BYTE, texture.get_pixel_data()[y]);
            } else {
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, static_cast<int32_t>(y), texture.width, 1, GL_RGBA,
                        GL_UNSIGNED_BYTE, texture.get_pixel_data()[y]);
            }
        }

        if (!AGLET_GL_ARB_direct_state_access) {
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        texture_res.release();

        state.prepared_textures.insert({ texture_uid, handle });
        state.material_textures.insert({ material_res.uid, texture_uid });
    }

    void deinit_texture(texture_handle_t texture) {
        glDeleteTextures(1, &texture);
    }

    void release_texture(RendererState &state, const std::string &texture_uid) {
        auto &textures = state.prepared_textures;
        if (auto existing_it = textures.find(texture_uid); existing_it != textures.end()) {
            auto new_rc = existing_it->second.release();
            if (new_rc == 0) {
                deinit_texture(existing_it->second);
                textures.erase(existing_it);
            }
            Logger::default_logger().debug("Released handle on texture %s (new refcount = %lu)", texture_uid.c_str(), new_rc);
        }
    }
}
