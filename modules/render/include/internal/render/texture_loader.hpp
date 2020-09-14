/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module resman
#include "argus/resman.hpp"

namespace argus {
    class PngTextureLoader : public ResourceLoader {
        private:
            void *const load(std::istream &stream, const size_t size) const override;

            void unload(void *const data_ptr) const override;

        public:
            PngTextureLoader();
    };
}
