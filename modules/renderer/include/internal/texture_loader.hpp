#pragma once

// module renderer
#include "argus/renderer.hpp"

// module resman
#include "argus/resource_manager.hpp"

namespace argus {

    class PngTextureLoader : public ResourceLoader {
        public:
            PngTextureLoader();

            void const *const load(std::istream const &stream) const override;

            void unload(void *const data_ptr) const override;
    };

}
