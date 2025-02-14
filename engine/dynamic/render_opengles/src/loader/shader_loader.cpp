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

#include "argus/lowlevel/debug.hpp"
#include "argus/lowlevel/macros.hpp"

#include "argus/resman/resource.hpp"
#include "argus/resman/resource_loader.hpp"

#include "argus/render/common/shader.hpp"
#include "argus/render/defines.h"

#include "internal/render_opengles/loader/shader_loader.hpp"

#include <istream>
#include <iterator>
#include <string>
#include <vector>

#include <cstdint>
#include <cstdio>

namespace argus {
    // forward declarations
    class ResourceManager;

    ShaderLoader::ShaderLoader():
        ResourceLoader({ RESOURCE_TYPE_SHADER_GLSL_VERT, RESOURCE_TYPE_SHADER_GLSL_FRAG }) {
    }

    Result<void *, ResourceError> ShaderLoader::load(ResourceManager &manager, const ResourcePrototype &proto,
            std::istream &stream, size_t size) {
        UNUSED(manager);
        UNUSED(size);

        std::string type;
        ShaderStage stage;

        if (proto.media_type == RESOURCE_TYPE_SHADER_GLSL_VERT) {
            type = SHADER_TYPE_GLSL;
            stage = ShaderStage::Vertex;
        } else if (proto.media_type == RESOURCE_TYPE_SHADER_GLSL_FRAG) {
            type = SHADER_TYPE_GLSL;
            stage = ShaderStage::Fragment;
        } else {
            // shouldn't ever happen
            crash("Unrecognized shader media type %s", proto.media_type.c_str());
        }

        std::vector<uint8_t> src(std::istreambuf_iterator<char>(stream), {});
        src.push_back('\0');

        return make_ok_result(new Shader(proto.uid, type, stage, src));
    }

    Result<void *, ResourceError> ShaderLoader::copy(ResourceManager &manager, const ResourcePrototype &proto,
            const void *src, std::optional<std::type_index> type) {
        UNUSED(manager);

        if (type.has_value() && type.value() != std::type_index(typeid(Shader))) {
            return make_err_result(ResourceErrorReason::UnexpectedReferenceType, proto);
        }

        // no dependencies to load so we can just do a blind copy

        return make_ok_result(new Shader(*static_cast<const Shader *>(src)));
    }

    void ShaderLoader::unload(void *const data_buf) {
        delete static_cast<Shader *>(data_buf);
    }

}
