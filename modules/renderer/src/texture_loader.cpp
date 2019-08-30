#include "argus/renderer.hpp"
#include "internal/renderer_defines.hpp"
#include "internal/texture_loader.hpp"

#include "internal/logging.hpp"

#include <istream>
#include <png.h>

#define RESOURCE_EXTENSION_PNG "png"

namespace argus {

    static void _read_stream(png_structp stream, png_bytep buf, png_size_t size) {
        static_cast<std::ifstream*>(png_get_io_ptr(stream))->read((char*) buf, size);
    }

    PngTextureLoader::PngTextureLoader():
            ResourceLoader(RESOURCE_TYPE_TEXTURE_PNG, {RESOURCE_EXTENSION_PNG}) {
    }

    void *const PngTextureLoader::load(std::istream &stream, const size_t size) const {
        unsigned char sig[8];
        try {
            stream.read((char*) sig, 8);
        } catch(std::exception const &ex) {
            _ARGUS_FATAL("Failed to read resource from stream: %s\n", ex.what());
        }

        if (png_sig_cmp(sig, 0, 8) != 0) {
            set_error("Invalid PNG file");
            return nullptr;
        }

        png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (png_ptr == nullptr) {
            set_error("Failed to create PNG read struct");
            return nullptr;
        }

        png_infop info_ptr = png_create_info_struct(png_ptr);
        if (info_ptr == nullptr) {
            png_destroy_read_struct(&png_ptr, nullptr, nullptr);

            set_error("Failed to create PNG info struct");
            return nullptr;
        }

        png_infop end_info_ptr = png_create_info_struct(png_ptr);
        if (info_ptr == nullptr) {
            png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);

            set_error("Failed to create PNG end info struct");
            return nullptr;
        }


        if (setjmp(png_jmpbuf(png_ptr)) != 0) {
            set_error("libpng setjmp call failed");
            return nullptr;
        }

        png_set_read_fn(png_ptr, static_cast<void*>(&stream), _read_stream);

        png_set_sig_bytes(png_ptr, 8);

        png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, nullptr);

        size_t width = png_get_image_width(png_ptr, info_ptr);
        size_t height = png_get_image_height(png_ptr, info_ptr);
        size_t bpp = png_get_bit_depth(png_ptr, info_ptr);
        size_t channels = png_get_channels(png_ptr, info_ptr);

        unsigned char *row_pointers[height];
        png_set_rows(png_ptr, info_ptr, row_pointers);

        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info_ptr);

        try {
            return new TextureData{width, height, bpp, channels, row_pointers};
        } catch (std::invalid_argument &ex) {
            set_error(ex.what());
            return nullptr;
        }
    }

    void PngTextureLoader::unload(void *const data_buf) const {
        delete static_cast<TextureData*>(data_buf);
    }

}
