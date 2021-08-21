/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "internal/lowlevel/logging.hpp"

// module resman
#include "argus/resman/resource.hpp"
#include "argus/resman/resource_manager.hpp"

// module render
#include "argus/render/common/material.hpp"
#include "argus/render/common/texture_data.hpp"
#include "internal/render/pimpl/common/material.hpp"
#include "internal/render/pimpl/common/texture_data.hpp"

// module render_opengl
#include "internal/render_opengl/types.hpp"
#include "internal/render_opengl/renderer/texture_mgmt.hpp"
#include "internal/render_opengl/state/renderer_state.hpp"

#include "aglet/aglet.h"

#include <map>
#include <string>
#include <utility>

#include <cstddef>

namespace argus {
    void prepare_texture(RendererState &state, const Resource &material_res) {
        auto &texture_uid = material_res.get<Material>().pimpl->texture;

        if (state.prepared_textures.find(texture_uid) != state.prepared_textures.end()) {
            return;
        }

        auto &texture_res = ResourceManager::get_global_resource_manager().get_resource_weak(texture_uid);
        auto &texture = texture_res.get<TextureData>();

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

        size_t row_size = texture.width * 32 / 8;
        if (AGLET_GL_ARB_direct_state_access) {
            glTextureStorage2D(handle, 1, GL_RGBA8, texture.width, texture.height);
        } else if (AGLET_GL_ARB_texture_storage) {
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, texture.width, texture.height);
        } else {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture.width, texture.height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                    nullptr);
        }

        size_t offset = 0;
        for (size_t y = 0; y < texture.height; y++) {
            if (AGLET_GL_ARB_direct_state_access) {
                glTextureSubImage2D(handle, 0, 0, y, texture.width, 1, GL_RGBA, GL_UNSIGNED_BYTE,
                        texture.pimpl->image_data[y]);
            } else {
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, y, texture.width, 1, GL_RGBA, GL_UNSIGNED_BYTE,
                        texture.pimpl->image_data[y]);
            }
            offset += row_size;
        }

        if (!AGLET_GL_ARB_direct_state_access) {
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        state.prepared_textures.insert({ texture_uid, handle });
    }

    void deinit_texture(texture_handle_t texture) {
        glDeleteTextures(1, &texture);
    }

    void remove_texture(RendererState &state, const std::string &texture_uid) {
        _ARGUS_DEBUG("De-initializing texture %s\n", texture_uid.c_str());
        auto &textures = state.prepared_textures;
        auto existing_it = textures.find(texture_uid);
        if (existing_it != textures.end()) {
            deinit_texture(existing_it->second);
            textures.erase(existing_it);
        }
    }
}
