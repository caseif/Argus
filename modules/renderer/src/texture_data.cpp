#include "argus/renderer.hpp"

namespace argus {

    TextureData::TextureData(const size_t width, const size_t height, const size_t bpp):
            width(width),
            height(height),
            bpp(bpp),
            data(new unsigned char[get_data_length()]) {
    }

    TextureData::~TextureData(void) {
        delete[] data;
    }

    const size_t TextureData::get_data_length(void) {
        width * height * bpp / 8;
    }

}
