/*
 * This file is a part of Argus.
 * Copyright (c) 2019, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

#include <cstddef>
#include <cstdint>

namespace argus {

    struct pimpl_AllocPool;

    class AllocPool {
        private:
            pimpl_AllocPool *pimpl;

        public:
            AllocPool(size_t block_size, uint8_t alignment_exp, size_t initial_cap);

            ~AllocPool(void);

            void *alloc(void);

            void free(void *addr);
    };

}
