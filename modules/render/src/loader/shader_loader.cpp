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
#include "argus/resman/resource_loader.hpp"

// module render
#include "argus/render/common/shader.hpp"
#include "internal/render/defines.hpp"
#include "internal/render/loader/shader_loader.hpp"

#include "png.h"
#include "pngconf.h"

#include <fstream> // IWYU pragma: keep
#include <istream> // IWYU pragma: keep
#include <stdexcept>
#include <utility>

#include <csetjmp>
#include <cstdio>

namespace argus {

    static void _read_stream(png_structp stream, png_bytep buf, png_size_t size) {
        static_cast<std::ifstream*>(png_get_io_ptr(stream))->read((char*) buf, size);
    }

    ShaderLoader::ShaderLoader():
            ResourceLoader({ RESOURCE_TYPE_SHADER_GLSL_VERT, RESOURCE_TYPE_SHADER_GLSL_FRAG }) {
    }

    void *const ShaderLoader::load(const ResourcePrototype &proto, std::istream &stream, const size_t size) const {
        ShaderStage stage;
        if (proto.media_type == RESOURCE_TYPE_SHADER_GLSL_VERT) {
            stage = ShaderStage::VERTEX;
        } else if (proto.media_type == RESOURCE_TYPE_SHADER_GLSL_FRAG) {
            stage = ShaderStage::FRAGMENT;
        } else {
            _ARGUS_FATAL("Unrecognized shader media type %s\n", proto.media_type);
        }

        std::string src(std::istreambuf_iterator<char>(stream), {});

        return new Shader(stage, src);
    }

    void ShaderLoader::unload(void *const data_buf) const {
        delete static_cast<Shader*>(data_buf);
    }

}
