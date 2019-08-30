#pragma once

// module renderer
#include "argus/renderer.hpp"

// module resman
#include "argus/resource_manager.hpp"

namespace argus {

    class PngTextureLoader : public ResourceLoader<TextureData> {
        public:
            PngTextureLoader();

            const TextureData load(std::istream const &stream) const override;
    };

}
