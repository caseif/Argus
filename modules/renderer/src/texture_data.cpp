// module lowlevel
#include "internal/logging.hpp"

// module renderer
#include "argus/renderer.hpp"
#include "internal/glext.hpp"

#include <SDL2/SDL_opengl.h>

namespace argus {

    using namespace glext;

    // IMPORTANT: image_data is assumed to be allocated on the heap
    TextureData::TextureData(const size_t width, const size_t height, const size_t bpp, const size_t channels,
            unsigned char **image_data):
            width(width),
            height(height),
            bpp(bpp),
            channels(channels),
            image_data(image_data),
            prepared(false) {
    }

    TextureData::~TextureData(void) {
        glDeleteBuffers(1, &buffer_handle);
    }

    const bool TextureData::is_prepared(void) {
        return prepared;
    }

    void TextureData::prepare(void) {
        if (get_pixel_format() == GL_FALSE) {
            throw std::invalid_argument("Unsupported pixel format for texture");
        }

        glGenBuffers(1, &buffer_handle);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, buffer_handle);

        if (!glIsBuffer(buffer_handle)) {
            _ARGUS_FATAL("Failed to gen pixel buffer during texture preparation\n");
        }

        size_t row_size = width * bpp * channels / 8;
        glBufferData(GL_PIXEL_UNPACK_BUFFER, height * row_size, nullptr, GL_STATIC_DRAW);

        size_t offset = 0;
        for (size_t y = 0; y < height; y++) {
            glBufferSubData(GL_PIXEL_UNPACK_BUFFER, offset, row_size, image_data[y]);
            offset += row_size;
        }

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

        for (size_t y = 0; y < height; y++) {
            delete[] image_data[y];
        }
        delete[] image_data;

        prepared = true;
    }

    const unsigned int TextureData::get_pixel_format(void) const {
        switch (channels) {
            case 1:
                switch (bpp) {
                    case 8:
                        return GL_R8;
                    case 16:
                        return GL_R16;
                    default:
                        return GL_FALSE;
                }
            case 2:
                switch (bpp) {
                    case 8:
                        return GL_RG8;
                    case 16:
                        return GL_RG16;
                    default:
                        return GL_FALSE;
                }
            case 3:
                switch (bpp) {
                    case 4:
                        return GL_RGB4;
                    case 5:
                        return GL_RGB5;
                    case 8:
                        return GL_RGB8;
                    case 10:
                        return GL_RGB10;
                    case 12:
                        return GL_RGB12;
                    case 16:
                        return GL_RGB16;
                    default:
                        return GL_FALSE;
                }
            case 4:
                switch (bpp) {
                    case 2:
                        return GL_RGBA2;
                    case 4:
                        return GL_RGBA4;
                    case 8:
                        return GL_RGBA8;
                    case 12:
                        return GL_RGBA12;
                    case 16:
                        return GL_RGBA16;
                    default:
                        return GL_FALSE;
                }
            default:
                return GL_FALSE;
        }
    }

}
