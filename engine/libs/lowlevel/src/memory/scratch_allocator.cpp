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

#include "argus/lowlevel/memory.hpp"
#include "internal/lowlevel/memory/util.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstdio>

namespace argus {

    // disable non-standard extension warning for zero-sized array member
    #ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable : 4200)
    #endif
    struct ScratchChunk {
        size_t m_size;
        ScratchChunk *m_prev_chunk;
        unsigned char m_data[0];
    };
    #ifdef _MSC_VER
    #pragma warning(pop)
    #endif

    //constexpr const size_t k_min_chunk_size = 4096;
    constexpr const size_t k_chunk_alignment_exp = 12;

    struct pimpl_ScratchAllocator {
        uint8_t m_alignment_exp;
        ScratchChunk *m_tail;
        uintptr_t m_next_addr;

        pimpl_ScratchAllocator(uint8_t alignment_exp) :
            m_alignment_exp(alignment_exp),
            m_tail(nullptr),
            m_next_addr(0) {
        }
    };

    static PoolAllocator g_pimpl_pool = PoolAllocator(sizeof(pimpl_ScratchAllocator));

    static void _alloc_chunk(pimpl_ScratchAllocator &pimpl, size_t min_space = 1) {
        size_t min_size = min_space + offsetof(ScratchChunk, m_data);
        size_t actual_size = next_aligned_value(min_size, k_chunk_alignment_exp);

        //TODO: use direct page allocation
        ScratchChunk *new_chunk = reinterpret_cast<ScratchChunk *>(malloc(actual_size));
        new_chunk->m_size = actual_size;
        new_chunk->m_prev_chunk = pimpl.m_tail;
        pimpl.m_tail = new_chunk;
        pimpl.m_next_addr = next_aligned_value(
                reinterpret_cast<uintptr_t>(&pimpl.m_tail->m_data), pimpl.m_alignment_exp);
    }

    ScratchAllocator::ScratchAllocator(uint8_t alignment_exp) :
        m_pimpl(&g_pimpl_pool.construct<pimpl_ScratchAllocator>(alignment_exp)) {
        _alloc_chunk(*m_pimpl);
    }

    ScratchAllocator::ScratchAllocator(const ScratchAllocator &rhs) :
        m_pimpl(&g_pimpl_pool.construct<pimpl_ScratchAllocator>(rhs.m_pimpl->m_alignment_exp)) {
    }

    ScratchAllocator::ScratchAllocator(ScratchAllocator &&rhs) noexcept :
            m_pimpl(rhs.m_pimpl) {
        rhs.m_pimpl = nullptr;
    }

    ScratchAllocator &ScratchAllocator::operator=(const ScratchAllocator &rhs) {
        if (&rhs == this) {
            return *this;
        }

        m_pimpl = &g_pimpl_pool.construct<pimpl_ScratchAllocator>(rhs.m_pimpl->m_alignment_exp);
        _alloc_chunk(*m_pimpl);

        return *this;
    }

    ScratchAllocator &ScratchAllocator::operator=(ScratchAllocator &&rhs) noexcept {
        if (&rhs == this) {
            return *this;
        }

        m_pimpl = rhs.m_pimpl;
        rhs.m_pimpl = nullptr;
        return *this;
    }

    ScratchAllocator::~ScratchAllocator(void) {
        if (m_pimpl == nullptr) {
            return;
        }

        release();

        g_pimpl_pool.destroy<pimpl_ScratchAllocator>(m_pimpl);
    }

    void *ScratchAllocator::alloc(size_t size) {
        size_t remaining = reinterpret_cast<uintptr_t>(m_pimpl->m_tail) + m_pimpl->m_tail->m_size - m_pimpl->m_next_addr;
        if (remaining < size) {
            _alloc_chunk(*m_pimpl, size);
        }
        void *addr = reinterpret_cast<void *>(m_pimpl->m_next_addr);
        m_pimpl->m_next_addr = next_aligned_value(m_pimpl->m_next_addr + size, m_pimpl->m_alignment_exp);
        return addr;
    }

    void ScratchAllocator::release(void) {
        ScratchChunk *cur_chunk = m_pimpl->m_tail;
        while (cur_chunk != nullptr) {
            ScratchChunk *next = cur_chunk->m_prev_chunk;
            //TODO: use direct page (de)allocation
            free(cur_chunk);
            cur_chunk = next;
        }
    }
}
