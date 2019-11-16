/*
 * This file is a part of Argus.
 * Copyright (c) 2019, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#include "argus/memory.hpp"

#include <list>
#include <stdexcept>

#include <cmath>
#include <cstdlib>

namespace argus {

    struct pimpl_AllocPool {
        size_t block_size;
        uint8_t alignment_exp;
        size_t chunk_size;
        size_t chunk_count;
        void *next_free_block;
        std::list<const uintptr_t> chunk_addrs; // used exclusively for destruction
    };

    // helper function for determining the nearest aligned address following the given unaligned address
    inline static uintptr_t _align_addr(uintptr_t addr, pimpl_AllocPool *pool) {
        size_t alignment_bytes = exp2(pool->alignment_exp);
        // We create a bitmask from the alignment "chunk" size by subtracting one (e.g. 0x0010 -> 0x000F)
        // and then inverting it (0x000F -> 0xFFF0). Then, we AND it with the real address to get the next
        // aligned address in the direction of zero, then add the alignment "chunk" size to get the next
        // aligned address in the direction of max size_t.
        return addr & ~(alignment_bytes - 1u) + alignment_bytes;
    }

    // helper function for allocating memory for an AllocPool
    static void *_create_chunk(pimpl_AllocPool *pool) {
        // because this function does a lot of pointer math, it mostly works with uintptr_t
        // to reduce the liklihood of mistakes related to type conversion

        size_t alignment_bytes = exp2(pool->alignment_exp);

        // the actual allocated size is the chunk size in bytes,
        // plus the maximum possible alignment padding (the alignment multiple minus 1)
        uintptr_t chunk_addr = reinterpret_cast<uintptr_t>(
                malloc(pool->block_size * pool->chunk_size + alignment_bytes - 1)
        );
        if (chunk_addr == 0) {
            throw std::runtime_error("Failed to allocate chunk (is block size or alignment too large?)");
        }

        pool->chunk_addrs.insert(pool->chunk_addrs.end(), chunk_addr);

        uintptr_t aligned_addr = _align_addr(chunk_addr, pool);

        // next we need to build a linked list within the allocated memory
        for (size_t index = 0; index < pool->chunk_size - 1; index++) {
            uintptr_t cur_block = aligned_addr + (index * pool->block_size);
            uintptr_t next_block = cur_block + pool->block_size;
            // write a pointer to the next block in the first n bytes of the current block
            // we reinterpret the block's memory as an array of pointers and set the first entry
            reinterpret_cast<uintptr_t*>(cur_block)[0] = next_block;
        }

        // insert a null pointer in the last block
        uintptr_t last_block = aligned_addr + ((pool->chunk_size - 1) * pool->block_size);
        reinterpret_cast<uintptr_t*>(last_block)[0] = 0;

        return reinterpret_cast<void*>(aligned_addr);
    }

    AllocPool::AllocPool(size_t block_size, uint8_t alignment_exp, size_t initial_cap):
            pimpl(new pimpl_AllocPool({
                block_size,
                alignment_exp,
                initial_cap,
                1,
                nullptr,
                {}
            })) {
        if (block_size < sizeof(size_t)) {
            throw std::invalid_argument("Block size too small");
        }

        void *first_chunk_addr = _create_chunk(pimpl);
        pimpl->next_free_block = first_chunk_addr;
    }

    AllocPool::~AllocPool(void) {
        for (uintptr_t chunk_ptr : pimpl->chunk_addrs) {
            free(reinterpret_cast<void*>(chunk_ptr));
        }
    }

    void *AllocPool::alloc(void) {
        if (pimpl->next_free_block == nullptr) {
            _create_chunk(pimpl);
        }

        // grab the next free block
        void *res = pimpl->next_free_block;

        // bump the linked list entry point along
        // again, we reinterpret the block's memory as an array of pointers and grab the first entry
        pimpl->next_free_block = reinterpret_cast<void**>(res)[0];

        return res;
    }

    void AllocPool::free(void *addr) {
        //TODO: validate address

        // reinterpret the freed block's memory as an array of pointers and set the first entry
        reinterpret_cast<void**>(addr)[0] = pimpl->next_free_block;
        pimpl->next_free_block = addr;

        //TODO: determine if we should deallocate any chunks
        //TODO: if any chunks are deallocated, the allocated blocks should be consolidated
    }

}
