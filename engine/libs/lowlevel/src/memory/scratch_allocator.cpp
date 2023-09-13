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

#include "argus/lowlevel/macros.hpp"
#include "argus/lowlevel/memory.hpp"

#include <cstddef>
#include <cstdint>

namespace argus {
    // disable non-standard extension warning for zero-sized array member
    #ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable : 4200)
    #endif
    struct ScratchChunk {
        size_t m_size;
        ScratchChunk *m_next_chunk;
        size_t m_cursor_off;
        unsigned char m_data[0];
    };
    #ifdef _MSC_VER
    #pragma warning(pop)
    #endif

    struct pimpl_ScratchAllocator {
        uint8_t m_alignment_exp;
        ScratchChunk *m_head;
    };

    ScratchAllocator::ScratchAllocator(uint8_t alignment_exp) :
        m_pimpl(nullptr) {
        UNUSED(m_pimpl);
        UNUSED(alignment_exp);
        //TODO
    }

    ScratchAllocator::~ScratchAllocator(void) {
        //TODO
    }

    void *ScratchAllocator::alloc(size_t size) {
        UNUSED(size);
        //TODO
        return nullptr;
    }

    void ScratchAllocator::release(void) {
        //TODO
    }
}
