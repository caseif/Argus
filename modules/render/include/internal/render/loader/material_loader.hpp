/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module resman
#include "argus/resman.hpp"

namespace argus {
    class MaterialLoader : public ResourceLoader {
        private:
            void *load(ResourceManager &manager, const ResourcePrototype &proto,
                    std::istream &stream, size_t size) const override;

            void unload(void *const data_ptr) const override;

        public:
            MaterialLoader();
    };
}
