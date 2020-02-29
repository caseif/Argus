/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#include "argus/memory.hpp"
#include "internal/lowlevel/logging.hpp"

#include <new>
#include <stdexcept>

#include <cmath>
#include <cstdlib>

#ifdef _MSC_VER
#include <intrin.h>
#endif

#define MAX(a, b) (a > b ? a : b)

#if defined(__LP64__) || defined(_WIN64)
#define BLOCKS_PER_CHUNK 64
#define BlockBitField uint64_t
// 6 bits set, enough to represent any index in a 64-bit bitfield
#define BF_INDEX_MASK 0x3F
#ifdef _MSC_VER
#define __bsr(i, m) _BitScanReverse64(reinterpret_cast<unsigned long*>(i), m)
#pragma intrinsic(_BitScanReverse64)
#else
#define __clz(x) __builtin_clzl(x)
#endif
#else
#define BLOCKS_PER_CHUNK 32
#define BlockBitField uint32_t
// 5 bits set, enough to represent any index in a 32-bit bitfield
#define BF_INDEX_MASK 0x1F
#ifdef _MSC_VER
#define __bsr(i, m) _BitScanReverse(reinterpret_cast<unsigned long*>(i), m)
#pragma intrinsic(_BitScanReverse)
#else
#define __clz(x) __builtin_clz(x)
#endif
#endif

#if _ARGUS_DEBUG_MODE
#define CANARY_LEN 4
#define CANARY_MAGIC 0xDEADD00D
typedef uint32_t CanaryValue;
#else
#define CANARY_LEN 0
#endif

namespace argus {

    // some quick terminology:
    //   block - a section of memory used to store a single object in the pool.
    //   chunk - a section of contiguous memory used to store multiple pool objects.
    //           a pool may contain many non-contiguous chunks.

    // disable non-standard extension warning for flexible array member
    #ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable: 4200)
    #endif
    struct ChunkMetadata {
        const uintptr_t unaligned_addr; // the address returned by malloc when creating the chunk
        size_t occupied_blocks; // the number of occupied blocks in the chunk, used for bookkeeping
        BlockBitField occupied_block_map; // a bitfield of blocks which are currently occupied
        ChunkMetadata *next_chunk;
        unsigned char data[];
    };
    #ifdef _MSC_VER
    #pragma warning(pop)
    #endif

    struct pimpl_AllocPool {
        const size_t nominal_block_size;
        const size_t real_block_size;
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

        size_t alignment_bytes = int(exp2(alignment_exp));
        // We create a bitmask from the alignment "chunk" size by subtracting one (e.g. 0x0010 -> 0x000F)
        // and then inverting it (0x000F -> 0xFFF0). Then, we AND it with the base address minus 1 to get
        // the next aligned address in the direction of zero, then add the alignment "chunk" size to get
        // the next aligned address in the direction of max size_t.
        // Subtracting one from the base address accounts for the case where the address is already aligned.
        return ((base_val - 1) & ~(alignment_bytes - 1u)) + alignment_bytes;
    }

    // helper function for allocating memory for an AllocPool
    static ChunkMetadata *_create_chunk(pimpl_AllocPool *pool) {
        // because this function does a lot of pointer math, it mostly works with uintptr_t
        // to reduce the likelihood of mistakes related to type conversion

        size_t alignment_bytes = int(exp2(pool->alignment_exp));

        // the actual allocated size is the chunk size in bytes,
        // plus the maximum possible alignment padding (the alignment multiple minus 1),
        // plus the size of the chunk metadata,
        // plus the canary length if we're in debug mode
        uintptr_t malloc_addr = reinterpret_cast<uintptr_t>(
                malloc(pool->real_block_size * pool->blocks_per_chunk
                + alignment_bytes - 1
                + sizeof(ChunkMetadata)
                + CANARY_LEN)
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
		new_chunk->occupied_block_map = 0;
		new_chunk->next_chunk = nullptr;

        #if _ARGUS_DEBUG_MODE
        CanaryValue *canary = reinterpret_cast<CanaryValue*>(
            new_chunk->data + pool->real_block_size * pool->blocks_per_chunk
        );
        *canary = CANARY_MAGIC;
        #endif

        return new_chunk;
    }

    void AllocPool::validate_block_size(size_t size) const {
        _ARGUS_ASSERT(size == pimpl->real_block_size, "Size mismatch for AllocPool");
    }

    AllocPool::AllocPool(size_t block_size, uint8_t alignment_exp):
            pimpl(new pimpl_AllocPool({
                block_size,
                _next_aligned_value(block_size, alignment_exp), // objects must be aligned within the pool
                alignment_exp,
                BLOCKS_PER_CHUNK,
                1,
                nullptr
            })) {
        //TODO: do we still need this?
        if (block_size < sizeof(size_t)) {
            throw std::invalid_argument("Block size too small");
        }

        ChunkMetadata *first_chunk = _create_chunk(pimpl);
        pimpl->first_chunk = first_chunk;
    }

    AllocPool::~AllocPool(void) {
        const ChunkMetadata *chunk = pimpl->first_chunk;
        while (chunk != NULL) {
            uintptr_t addr = chunk->unaligned_addr;
            ::free(reinterpret_cast<void*>(addr));
            chunk = chunk->next_chunk;
        }
    }

    void *AllocPool::alloc(void) {
        //return malloc(pimpl->real_block_size); // for benchmarking purposes
        ChunkMetadata *cur_chunk = pimpl->first_chunk;
        ChunkMetadata *selected_chunk = nullptr;
        size_t max_block_count = 0;
        // iterate the chunks and pick the one with the highest block count.
        // this way, we can avoid excessive fragmentation.
        while (cur_chunk != nullptr) {
            // SIZE_MAX works because the block map is guaranteed to be size_t bytes,
            // so SIZE_MAX represents a filled bitfield
            if (cur_chunk->occupied_block_map != SIZE_MAX
                    && cur_chunk->occupied_blocks >= max_block_count) {
                selected_chunk = cur_chunk;
            }
            cur_chunk = cur_chunk->next_chunk;
        }

        if (selected_chunk == nullptr) {
            // need to allocate a new chunk
            selected_chunk = _create_chunk(pimpl);
            selected_chunk->next_chunk = pimpl->first_chunk;
            pimpl->first_chunk = selected_chunk;
        }

        // this is ultimately set to the sequential memory index of the first free block
        // note that this index is the reverse of the position of the corresponding bit in the bitfield
        // e.g. the first block has index 0 and is flagged by the MSB at position 63 (on x64)
        size_t first_free_block_index = 0;
        #ifdef _MSC_VER
        // returns the bit position of the first set bit (clear in this case, since we invert the bitfield)
        // if the MSB is clear in the original bitfield, this returns 63 on x64
        // if all except the LSB are clear in the original bitfield, this returns 0 on x64
        __bsr(&first_free_block_index, ~selected_chunk->occupied_block_map);
        // we actually need the one's-complement to convert the bit position to a sequential index
        // in order to match behavior on UNIX
        // we also need to mask it because the higher bits are irrelevant when addressing the bitfield
        first_free_block_index = (~first_free_block_index & BF_INDEX_MASK);
        #else
        // returns the number of leading clear bits (leading set bits in this case, since we invert the bitfield)
        // if the MSB is clear in the original bitfield, this returns 0
        // if all except the LSB are clear in the original bitfield, this returns 63 on x64
        first_free_block_index = __clz(~selected_chunk->occupied_block_map);
        #endif

        // set the relevant bit in the block map
        // we convert the index to a bit position by taking the one's-complement and masking
        // it to exclude bits not relevant when addressing the bitfield
        selected_chunk->occupied_block_map |= (BlockBitField(1) << (~first_free_block_index & BF_INDEX_MASK));

        uintptr_t block_addr = reinterpret_cast<uintptr_t>(selected_chunk->data)
                + (first_free_block_index * pimpl->real_block_size);

        selected_chunk->occupied_blocks += 1;

        return reinterpret_cast<void*>(block_addr);
    }

    void AllocPool::free(void *const addr) {
        const size_t chunk_len = pimpl->real_block_size * pimpl->blocks_per_chunk;
        
        // keep track of the last chunk in case we have to remove one
        ChunkMetadata *last_chunk = nullptr;
        
        ChunkMetadata *chunk = pimpl->first_chunk;
        while (chunk != nullptr) {
            if (addr >= chunk->data && addr < chunk->data + chunk_len) {
                size_t offset_in_chunk = reinterpret_cast<uintptr_t>(addr) - reinterpret_cast<uintptr_t>(chunk->data);
                if ((offset_in_chunk % pimpl->real_block_size) != 0) {
                    throw std::invalid_argument("Pointer does not point to a valid block");
                }

                size_t block_index = offset_in_chunk / pimpl->real_block_size;
                // clear appropriate bit in block map
                // we convert the index to a bit position by taking the one's-complement and masking
                // it to exclude bits not relevant when addressing the bitfield
                chunk->occupied_block_map &= ~(1 << (~block_index & BF_INDEX_MASK));

                // if the chunk is empty, delete it
                if (--chunk->occupied_blocks == 0) {
                    bool should_delete = true;

                    // if this is the head, delete it and make the next one the new head
                    if (last_chunk == nullptr) {
                        // don't delete the last remaining chunk
                        if (chunk->next_chunk != nullptr) {
                            pimpl->first_chunk = chunk->next_chunk;
                        } else {
                            should_delete = false;
                        }
                    } else {
                        // otherwise, simply delete the node
                        last_chunk->next_chunk = chunk->next_chunk;
                    }

                    if (should_delete) {
                        #if _ARGUS_DEBUG_MODE
                        CanaryValue *canary = reinterpret_cast<CanaryValue*>(
                            chunk->data + pimpl->real_block_size * pimpl->blocks_per_chunk
                        );
                        if (*canary != CANARY_MAGIC) {
                            _ARGUS_FATAL("Detected heap overrun in chunk @ %p (aligned: %p)",
                                reinterpret_cast<void*>(chunk->unaligned_addr),
                                chunk);
                        }
                        #endif

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
