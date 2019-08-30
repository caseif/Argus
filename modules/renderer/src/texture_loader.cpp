#include "argus/renderer.hpp"
#include "internal/renderer_defines.hpp"
#include "internal/texture_loader.hpp"

#define RESOURCE_EXTENSION_PNG "png"

namespace argus {

    PngTextureLoader::PngTextureLoader():
            ResourceLoader({RESOURCE_TYPE_TEXTURE_PNG}, {RESOURCE_EXTENSION_PNG}) {
    }

    void const *const PngTextureLoader::load(std::istream const &stream) const {
        printf("called\n");
        return new TextureData{1, 1, 1};
    }

    void PngTextureLoader::unload(void *const data_buf) const {
        delete static_cast<TextureData*>(data_buf);
    }

}
