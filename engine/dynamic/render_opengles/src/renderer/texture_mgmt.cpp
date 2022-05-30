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
#include "internal/render/pimpl/common/material.hpp"
#include "internal/render/pimpl/common/texture_data.hpp"

#include "internal/render_opengles/types.hpp"
#include "internal/render_opengles/renderer/texture_mgmt.hpp"
#include "internal/render_opengles/state/renderer_state.hpp"

#include "aglet/aglet.h"

#include <map>
#include <string>
#include <utility>

#include <cstddef>
#include <cstdint>

namespace argus {
    void prepare_texture(RendererState &state, const Resource &material_res) {
        auto &texture_uid = material_res.get<Material>().pimpl->texture;

        if (state.prepared_textures.find(texture_uid) != state.prepared_textures.end()) {
            return;
        }

        auto &texture_res = ResourceManager::instance().get_resource_weak(texture_uid);
        auto &texture = texture_res.get<TextureData>();
        
        _ARGUS_ASSERT(texture.width <= INT32_MAX, "Texture width is too big");
        _ARGUS_ASSERT(texture.height <= INT32_MAX, "Texture height is too big");

        texture_handle_t handle;

        glGenTextures(1, &handle);
        glBindTexture(GL_TEXTURE_2D, handle);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        //TODO: evaluate if this is a suitable replacement for GL_CLAMP_TO_BORDER
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        //glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

        if (AGLET_GL_ES_VERSION_3_0) {
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, texture.width, texture.height);
        } else {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture.width, texture.height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                    nullptr);
        }

        for (uint32_t y = 0; y < texture.height; y++) {
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, static_cast<int32_t>(y), texture.width, 1, GL_RGBA,
                    GL_UNSIGNED_BYTE, texture.pimpl->image_data[y]);
        }

        glBindTexture(GL_TEXTURE_2D, 0);

        state.prepared_textures.insert({ texture_uid, handle });
    }

    void deinit_texture(texture_handle_t texture) {
        glDeleteTextures(1, &texture);
    }

    void remove_texture(RendererState &state, const std::string &texture_uid) {
        _ARGUS_DEBUG("De-initializing texture %s", texture_uid.c_str());
        auto &textures = state.prepared_textures;
        auto existing_it = textures.find(texture_uid);
        if (existing_it != textures.end()) {
            deinit_texture(existing_it->second);
            textures.erase(existing_it);
        }
    }
}
