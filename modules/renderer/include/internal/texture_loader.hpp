#pragma once

// module renderer
#include "argus/renderer.hpp"

// module resman
#include "argus/resource_manager.hpp"

namespace argus {

    class PngTextureLoader : public ResourceLoader {
        private:
            void *const load(std::istream &stream, const size_t size) const override;

            void unload(void *const data_ptr) const override;

        public:
            PngTextureLoader();
    };

}
