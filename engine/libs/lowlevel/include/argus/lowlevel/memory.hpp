/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2023, Max Roncace <mproncace@protonmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <mutex>
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
        std::mutex alloc_mutex;

        explicit AllocPool(size_t block_size, uint8_t alignment_exp = 3);

        AllocPool(AllocPool &) = delete;

        AllocPool(AllocPool &&) = delete;

        AllocPool &operator=(AllocPool &) = delete;

        AllocPool &operator=(AllocPool &&) = delete;

        ~AllocPool(void);

        void *alloc(void);

        void free(void *addr);

        template<typename T, typename... Args>
        T &construct(Args &&... args) {
            return *new(this->alloc()) T(args...);
        }

        template<typename T>
        void destroy(T *obj) {
            //TODO: add a safeguard to prevent destructor being invoked on invalid pointer
            obj->~T();
            this->free(obj);
        }
    };

}
