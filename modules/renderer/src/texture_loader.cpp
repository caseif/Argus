#include "argus/renderer.hpp"
#include "internal/renderer_defines.hpp"
#include "internal/texture_loader.hpp"

#define RESOURCE_EXTENSION_PNG "png"

namespace argus {

    PngTextureLoader::PngTextureLoader():
            ResourceLoader<TextureData>({RESOURCE_TYPE_TEXTURE_PNG}, {RESOURCE_EXTENSION_PNG}) {
    }

    const TextureData PngTextureLoader::load(std::istream const &stream) const {
        printf("called\n");
        return {1, 1, 1};
    }

}
