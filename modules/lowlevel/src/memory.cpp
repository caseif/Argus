/*
 * This file is a part of Argus.
 * Copyright (c) 2019, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#include "argus/memory.hpp"
#include "internal/logging.hpp"

#include <list>
#include <stdexcept>

#include <cmath>
#include <cstdlib>

#define MAX(a, b) (a > b ? a : b)

namespace argus {

    // some quick terminology:
    //   block - a section of memory used to store a single object in the pool.
    //   chunk - a section of contiguous memory used to store multiple pool objects.
    //           a pool may contain many non-contiguous chunks.

    struct ChunkMetadata {
        const uintptr_t unaligned_addr; // the address returned by malloc when creating the chunk
        size_t occupied_blocks; // the number of occupied blocks in the chunk, used for bookkeeping
        ChunkMetadata *next_chunk;
        uintptr_t first_free_block;
        unsigned char data[];
    };

    struct pimpl_AllocPool {
        const size_t nominal_block_size;
        const size_t real_block_size;
        size_t alloced_block_count;
        const uint8_t alignment_exp;
        const size_t blocks_per_chunk;
        size_t chunk_count;
        ChunkMetadata *first_chunk;
    };

    // helper function for determining the nearest aligned address following the given unaligned address
    inline static uintptr_t _next_aligned_value(uintptr_t base_val, size_t alignment_exp) {
        // special case if no alignment is requested
        if (alignment_exp == 0) {
            return base_val;
        }

        size_t alignment_bytes = exp2(alignment_exp);
        // We create a bitmask from the alignment "chunk" size by subtracting one (e.g. 0x0010 -> 0x000F)
        // and then inverting it (0x000F -> 0xFFF0). Then, we AND it with the real address to get the next
        // aligned address in the direction of zero, then add the alignment "chunk" size to get the next
        // aligned address in the direction of max size_t.
        return (base_val & ~(alignment_bytes - 1u)) + alignment_bytes;
    }

    // helper function for allocating memory for an AllocPool
    static ChunkMetadata *_create_chunk(pimpl_AllocPool *pool) {
        // because this function does a lot of pointer math, it mostly works with uintptr_t
        // to reduce the likelihood of mistakes related to type conversion

        size_t alignment_bytes = exp2(pool->alignment_exp);

        // the actual allocated size is the chunk size in bytes,
        // plus the maximum possible alignment padding (the alignment multiple minus 1),
        // plus the size of the chunk metadata
        uintptr_t malloc_addr = reinterpret_cast<uintptr_t>(
                malloc(pool->real_block_size * pool->blocks_per_chunk + alignment_bytes - 1 + sizeof(ChunkMetadata))
        );
        if (malloc_addr == reinterpret_cast<uintptr_t>(nullptr)) {
            throw std::runtime_error("Failed to allocate chunk (is block size or alignment too large?)");
        }

        uintptr_t aligned_addr = _next_aligned_value(malloc_addr, pool->alignment_exp);
        // the actual data starts at the aligned address. however, we use the preceding n bytes to store the metadata.
        // we need a big enough buffer between the malloc address and aligned address so we can fit the metadata.
        // otherwise, we have to bump up to the next multiple (it's guaranteed to fit anyhow).
        if (aligned_addr - malloc_addr < sizeof(ChunkMetadata)) {
            // we need to get the max of the alignment padding vs the aligned size of the metadata, since if the
            // alignment padding is smaller we won't jump far enough ahead to fit the metadata
            aligned_addr += MAX(alignment_bytes, _next_aligned_value(sizeof(ChunkMetadata), pool->alignment_exp));
        }
        // generate a pointer to the now properly aligned chunk structure
        ChunkMetadata *new_chunk = reinterpret_cast<ChunkMetadata*>(aligned_addr - sizeof(ChunkMetadata));

        *const_cast<uintptr_t*>(&new_chunk->unaligned_addr) = malloc_addr;
        new_chunk->occupied_blocks = 0;
        new_chunk->first_free_block = reinterpret_cast<uintptr_t>(new_chunk->data);

        // next we need to build a linked list within the allocated memory
        for (size_t index = 0; index < pool->blocks_per_chunk - 1; index++) {
            uintptr_t cur_block = reinterpret_cast<uintptr_t>(new_chunk->data) + (index * pool->real_block_size);
            uintptr_t next_block = cur_block + pool->real_block_size;
            // write a pointer to the next block in the first n bytes of the current block
            // we reinterpret the block's memory as an array of pointers and set the first entry
            reinterpret_cast<uintptr_t*>(cur_block)[0] = next_block;
        }

        // insert a null pointer in the last block
        uintptr_t last_block = reinterpret_cast<uintptr_t>(new_chunk->data)
                + ((pool->blocks_per_chunk - 1) * pool->real_block_size);
        reinterpret_cast<uintptr_t*>(last_block)[0] = 0;

        return new_chunk;
    }

    void AllocPool::validate_block_size(size_t size) const {
        _ARGUS_ASSERT(size == pimpl->real_block_size, "Size mismatch for AllocPool");
    }

    AllocPool::AllocPool(size_t block_size, size_t initial_cap, uint8_t alignment_exp):
            pimpl(new pimpl_AllocPool({
                block_size,
                _next_aligned_value(block_size, alignment_exp), // objects must be aligned within the pool
                0,
                alignment_exp,
                initial_cap,
                1,
                nullptr
            })) {
        if (block_size < sizeof(size_t)) {
            throw std::invalid_argument("Block size too small");
        }

        ChunkMetadata *first_chunk = _create_chunk(pimpl);
        pimpl->first_chunk = first_chunk;
    }

    AllocPool::~AllocPool(void) {
        const ChunkMetadata *chunk = this->pimpl->first_chunk;
        while (chunk != NULL) {
            uintptr_t addr = chunk->unaligned_addr;
            chunk = chunk->next_chunk;
            free(reinterpret_cast<void*>(addr));
        }
    }

    void *AllocPool::alloc(void) {
        //return malloc(this->pimpl->real_block_size);
        ChunkMetadata *cur_chunk = this->pimpl->first_chunk;
        ChunkMetadata *selected_chunk = nullptr;
        size_t max_block_count = 0;
        // iterate the chunks and pick the one with the highest block count.
        // this way, we can avoid excessive fragmentation.
        while (cur_chunk != nullptr) {
            if (cur_chunk->occupied_blocks < this->pimpl->blocks_per_chunk
                    && cur_chunk->occupied_blocks >= max_block_count) {
                selected_chunk = cur_chunk;
            }
            cur_chunk = cur_chunk->next_chunk;
        }

        if (selected_chunk == nullptr) {
            // need to allocate a new chunk
            selected_chunk = _create_chunk(pimpl);
            selected_chunk->next_chunk = this->pimpl->first_chunk;
            this->pimpl->first_chunk = selected_chunk;
        }

        uintptr_t block_addr = selected_chunk->first_free_block;

        // bump the linked list entry point along
        // reinterpret the block's memory as an array of pointers and grab the first entry
        selected_chunk->first_free_block = reinterpret_cast<uintptr_t*>(block_addr)[0];

        pimpl->alloced_block_count += 1;
        selected_chunk->occupied_blocks += 1;

        return reinterpret_cast<void*>(block_addr);
    }

    void AllocPool::free(void *const addr) {
        const size_t chunk_len = this->pimpl->real_block_size * this->pimpl->blocks_per_chunk;
        
        // keep track of the last chunk in case we have to remove one
        ChunkMetadata *last_chunk = nullptr;
        
        ChunkMetadata *chunk = this->pimpl->first_chunk;
        while (chunk != nullptr) {
            if (addr >= chunk->data && addr < chunk->data + chunk_len) {
                size_t offset_in_chunk = reinterpret_cast<uintptr_t>(addr) - reinterpret_cast<uintptr_t>(chunk->data);
                if ((offset_in_chunk % this->pimpl->real_block_size) != 0) {
                    throw std::invalid_argument("Pointer does not point to a valid block");
                }

                // reinterpret the freed block's memory as an array of pointers and store the current block list head
                reinterpret_cast<uintptr_t*>(addr)[0] = chunk->first_free_block;
                // then update the head to point to the freed block
                chunk->first_free_block = reinterpret_cast<uintptr_t>(addr);

                // if the chunk is empty, delete it
                if (--chunk->occupied_blocks == 0) {
                    bool should_delete = true;

                    // if this is the head, delete it and make the next one the new head
                    if (last_chunk == nullptr) {
                        // don't delete the last remaining chunk
                        if (chunk->next_chunk != nullptr) {
                            this->pimpl->first_chunk = chunk->next_chunk;
                        } else {
                            should_delete = false;
                        }
                    } else {
                        // otherwise, simply delete the node
                        last_chunk->next_chunk = chunk->next_chunk;
                    }

                    if (should_delete) {
                        ::free(reinterpret_cast<void*>(chunk->unaligned_addr));
                    }
                }

                return;
            }

            last_chunk = chunk;
            chunk = chunk->next_chunk;
        }
        throw std::invalid_argument("Pointer is not contained by a chunk");
    }

}
