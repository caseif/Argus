// module lowlevel
#include "internal/logging.hpp"

// module renderer
#include "argus/renderer.hpp"

#include <SDL2/SDL_opengl.h>

namespace argus {

    static GLenum get_format(size_t bpp, size_t channels, bool *fail) {
        switch (channels) {
            case 1:
                switch (bpp) {
                    case 8:
                        return GL_R8;
                    case 16:
                        return GL_R16;
                    default:
                        break;
                }
            case 2:
                switch (bpp) {
                    case 8:
                        return GL_RG8;
                    case 16:
                        return GL_RG16;
                    default:
                        break;
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
                        break;
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
                        break;
                }
            default:
                break;
        }
        *fail = true;
    }

    TextureData::TextureData(const size_t width, const size_t height, const size_t bpp, const size_t channels,
            unsigned char **image_data):
            width(width),
            height(height),
            bpp(bpp),
            channels(channels),
            ready(false) {
        //TODO: not sure this is the most efficient way to do this
        this->image_data = new unsigned char*[height];
        for (size_t y = 0; y < height; y++) {
            this->image_data[y] = new unsigned char[width];
            image_data[y] = std::move(image_data[y]);
        }
    }

    TextureData::~TextureData(void) {
        glDeleteTextures(1, &tex_handle);
    }

    const bool TextureData::is_ready(void) {
        return ready;
    }

    void TextureData::upload_to_gpu(void) {
        glGenTextures(1, &tex_handle);
        if (!glIsTexture(tex_handle)) {
            _ARGUS_FATAL("Failed to gen GL texture: %d\n", glGetError());
        }

        printf("genned tex %d\n", tex_handle);
        glBindTexture(GL_TEXTURE_2D, tex_handle);

        bool fail = false;
        GLenum format = get_format(bpp, channels, &fail);
        if (fail) {
            throw std::invalid_argument("Unsupported pixel format for texture");
        }

        for (size_t y = 0; y < height; y++) {
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, y, width, 1, format, GL_UNSIGNED_BYTE, image_data[y]);
        }

        glBindTexture(GL_TEXTURE_2D, 0);

        for (size_t y = 0; y < height; y++) {
            delete[] image_data[y];
        }
        delete[] image_data;

        ready = true;
    }

}
