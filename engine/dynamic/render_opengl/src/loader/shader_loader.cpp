/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@protonmail.com>
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

// module lowlevel
#include "argus/lowlevel/macros.hpp"
#include "internal/lowlevel/logging.hpp"

// module resman
#include "argus/resman/resource.hpp"
#include "argus/resman/resource_loader.hpp"

// module render
#include "argus/render/common/shader.hpp"

// module render_opengl
#include "internal/render_opengl/defines.hpp"
#include "internal/render_opengl/loader/shader_loader.hpp"

#include <istream>
#include <iterator>
#include <string>

#include <cstdio>

namespace argus {
    // forward declarations
    class ResourceManager;

    ShaderLoader::ShaderLoader():
            ResourceLoader({ RESOURCE_TYPE_SHADER_GLSL_VERT, RESOURCE_TYPE_SHADER_GLSL_FRAG }) {
    }

    void *ShaderLoader::load(ResourceManager &manager, const ResourcePrototype &proto,
            std::istream &stream, size_t size) const {
        UNUSED(manager);
        UNUSED(size);
        ShaderStage stage;
        if (proto.media_type == RESOURCE_TYPE_SHADER_GLSL_VERT) {
            stage = ShaderStage::Vertex;
        } else if (proto.media_type == RESOURCE_TYPE_SHADER_GLSL_FRAG) {
            stage = ShaderStage::Fragment;
        } else {
            _ARGUS_FATAL("Unrecognized shader media type %s\n", proto.media_type.c_str());
        }

        std::string src(std::istreambuf_iterator<char>(stream), {});

        return new Shader(stage, src);
    }

    void ShaderLoader::unload(void *const data_buf) const {
        delete static_cast<Shader*>(data_buf);
    }

}