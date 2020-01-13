/*
 * This file is a part of Argus.
 * Copyright (c) 2019, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

#include <new>

#include <cstddef>
#include <cstdint>

namespace argus {

    void *alloc_page(void);

    struct pimpl_AllocPool;

    class AllocPool {
        private:
            pimpl_AllocPool *pimpl;

            void validate_block_size(size_t size) const;

        public:
            AllocPool(size_t block_size, size_t initial_cap, uint8_t alignment_exp = 3);

            ~AllocPool(void);

            void *alloc(void);

            template <typename T, typename... Args>
            T &construct(Args & ... args) {
                return *new (this->alloc()) T(args...);
            }

            void free(void *addr);
    };

}
