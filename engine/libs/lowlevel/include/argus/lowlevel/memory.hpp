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

#include "argus/lowlevel/macros.hpp"

#include <memory>
#include <mutex>
#include <new>

#include <cstddef>
#include <cstdint>

namespace argus {

    void *alloc_page(void);

    struct pimpl_PoolAllocator;

    class PoolAllocator {
      private:
        pimpl_PoolAllocator *pimpl;

      public:
        std::mutex alloc_mutex;

        explicit PoolAllocator(size_t block_size, uint8_t alignment_exp = 3);

        PoolAllocator(const PoolAllocator &) = delete;

        PoolAllocator(PoolAllocator &&) = delete;

        PoolAllocator &operator=(const PoolAllocator &) = delete;

        PoolAllocator &operator=(PoolAllocator &&) = delete;

        ~PoolAllocator(void);

        void *alloc(void);

        void free(void *addr);

        template <typename T, typename... Args>
        T &construct(Args &&... args) {
            return *new(this->alloc()) T(args...);
        }

        template <typename T>
        void destroy(T *obj) {
            //TODO: add a safeguard to prevent destructor being invoked on invalid pointer
            obj->~T();
            this->free(obj);
        }
    };

    struct pimpl_ScratchAllocator;

    class ScratchAllocator {
      private:
        pimpl_ScratchAllocator *m_pimpl;

      public:

        explicit ScratchAllocator(uint8_t alignment_exp = 3);

        ScratchAllocator(const ScratchAllocator &);

        ScratchAllocator(ScratchAllocator &&) noexcept;

        ScratchAllocator &operator=(const ScratchAllocator &);

        ScratchAllocator &operator=(ScratchAllocator &&) noexcept;

        ~ScratchAllocator(void);

        [[nodiscard]] void *alloc(size_t size);

        void release(void);

        template <typename T, typename... Args>
        [[nodiscard]] T &construct(Args &&... args) {
            return *new(this->alloc(sizeof(T))) T(std::forward<Args>(args)...);
        }
    };

    template <typename T>
    class ScratchAllocatorWrapper {
      private:
        ScratchAllocator &m_impl;

      public:
        using value_type = T;

        ScratchAllocatorWrapper(ScratchAllocator &impl) :
            m_impl(impl) {
        }

        template <typename U>
        constexpr ScratchAllocatorWrapper(const ScratchAllocatorWrapper<U> &rhs) noexcept :
            m_impl(rhs.m_impl) {
        }

        [[nodiscard]] T *allocate(size_t size) {
            return reinterpret_cast<T *>(m_impl.alloc(size));
        }

        void deallocate(T *ptr, size_t size) {
            // no-op
            UNUSED(ptr);
            UNUSED(size);
        }

        template <typename U>
        inline bool operator==(const ScratchAllocatorWrapper<U> &rhs) {
            return m_impl == rhs.m_impl;
        }

        template <typename U>
        inline bool operator != (const ScratchAllocatorWrapper<U> &rhs) {
            return !(*this == rhs);
        }
    };
}
