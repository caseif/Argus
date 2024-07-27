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

#include "argus/resman/resource_loader.hpp"
#include "argus/resman/resource_manager.hpp"

#include "argus/render/defines.h"
#include "argus/render/common/texture_data.hpp"
#include "internal/render/loader/texture_loader.hpp"

#include "png.h"
#include "pngconf.h"

#include <fstream> // IWYU pragma: keep
#include <istream>
#include <utility>

#include <csetjmp>
#include <cstdint>
#include <cstdio>

namespace argus {
    // forward declarations
    class ResourceManager;

    struct ResourcePrototype;

    static void _read_stream(png_structp stream, png_bytep buf, png_size_t size) {
        affirm_precond(size <= LONG_MAX, "PNG size is too large");
        static_cast<std::ifstream *>(png_get_io_ptr(stream))->read(reinterpret_cast<char *>(buf),
                std::streamsize(size));
    }

    PngTextureLoader::PngTextureLoader():
        ResourceLoader({ RESOURCE_TYPE_TEXTURE_PNG }) {
    }

    Result<void *, ResourceError> PngTextureLoader::load(ResourceManager &manager, const ResourcePrototype &proto,
            std::istream &stream, size_t size) {
        UNUSED(manager);
        UNUSED(proto);
        UNUSED(size);
        unsigned char sig[8];
        stream.read(reinterpret_cast<char *>(sig), 8);

        if (png_sig_cmp(sig, 0, 8) != 0) {
            return make_err_result(ResourceErrorReason::MalformedContent, proto, "Invalid PNG file");
        }

        png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
        if (png_ptr == nullptr) {
            crash("png_create_info_struct failed");
        }

        png_infop info_ptr = png_create_info_struct(png_ptr);
        if (info_ptr == nullptr) {
            png_destroy_read_struct(&png_ptr, nullptr, nullptr);

            crash("png_create_info_struct failed");
        }

        png_infop end_info_ptr = png_create_info_struct(png_ptr);
        if (end_info_ptr == nullptr) {
            png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);

            crash("png_create_info_struct failed");
        }

        #ifdef _MSC_VER
        #pragma warning(push, 3)
        #pragma warning(disable: 4611)
        #endif
        if (setjmp(png_jmpbuf(png_ptr)) != 0) { // NOLINT(*-err52-cpp)
            crash("libpng setjmp failed");
        }
        #ifdef _MSC_VER
        #pragma warning(pop)
        #endif

        png_set_read_fn(png_ptr, static_cast<void *>(&stream), _read_stream);

        png_set_sig_bytes(png_ptr, 8);

        png_read_info(png_ptr, info_ptr);

        auto width = png_get_image_width(png_ptr, info_ptr);
        auto height = png_get_image_height(png_ptr, info_ptr);
        auto bit_depth = png_get_bit_depth(png_ptr, info_ptr);
        auto color_type = png_get_color_type(png_ptr, info_ptr);

        if (width > INT32_MAX || height > INT32_MAX) {
            return make_err_result(ResourceErrorReason::UnsupportedContent, proto,
                    "Texture dimensions are too large (max 2147483647 pixels)");
        }

        if (bit_depth == 16) {
            png_set_strip_16(png_ptr);
        } else if (bit_depth < 8) {
            png_set_packing(png_ptr);
        }

        if (color_type == PNG_COLOR_TYPE_RGB) {
            png_set_add_alpha(png_ptr, 0xffffffff, PNG_FILLER_AFTER);
        }

        if (color_type == PNG_COLOR_TYPE_PALETTE) {
            png_set_palette_to_rgb(png_ptr);
        }

        if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
            png_set_expand_gray_1_2_4_to_8(png_ptr);
        }

        if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
            png_set_tRNS_to_alpha(png_ptr);
        }

        if (color_type == PNG_COLOR_TYPE_RGB
                || color_type == PNG_COLOR_TYPE_GRAY
                || color_type == PNG_COLOR_TYPE_PALETTE) {
            png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
        }

        if (color_type == PNG_COLOR_TYPE_GRAY
                || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
            png_set_gray_to_rgb(png_ptr);
        }

        png_read_update_info(png_ptr, info_ptr);

        unsigned char **row_pointers = new unsigned char *[sizeof(png_bytep) * height];
        for (uint32_t y = 0; y < height; y++) {
            row_pointers[y] = new unsigned char[png_get_rowbytes(png_ptr, info_ptr)];
        }

        png_read_image(png_ptr, row_pointers);

        png_read_end(png_ptr, end_info_ptr);

        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info_ptr);

        return make_ok_result(new TextureData { width, height, std::move(row_pointers) });
    }

    Result<void *, ResourceError> PngTextureLoader::copy(ResourceManager &manager, const ResourcePrototype &proto,
            void *src, std::type_index type) {
        UNUSED(manager);

        if (type != std::type_index(typeid(TextureData))) {
            return make_err_result(ResourceErrorReason::UnexpectedReferenceType, proto);
        }

        // no dependencies to load so we can just do a blind copy

        return make_ok_result(new TextureData(*static_cast<TextureData *>(src)));
    }

    void PngTextureLoader::unload(void *const data_buf) {
        delete static_cast<TextureData *>(data_buf);
    }

}
