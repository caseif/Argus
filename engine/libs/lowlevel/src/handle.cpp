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

#include "argus/lowlevel/handle.hpp"
#include "argus/lowlevel/logging.hpp"
#include "internal/lowlevel/handle.hpp"
#include "internal/lowlevel/pimpl/handle.hpp"

#include <random>
#include <stack>

#include <cfloat>

namespace argus {
    // Handle implementation

    bool Handle::operator==(const Handle &rhs) const {
        return this->index == rhs.index && this->uid == rhs.uid;
    }

    bool Handle::operator<(const Handle &rhs) const {
        return this->index < rhs.index || (this->index == rhs.index && this->uid < rhs.uid);
    }

    Handle::operator uint64_t(void) const {
        return (uint64_t(this->index) << 32) | uint64_t(this->uid);
    }

    // HandleTable implementation

    static std::mt19937 rand_mt((std::random_device()) ());
    static std::uniform_real_distribution<double> rand_distr(0, std::nextafter(UINT32_MAX, DBL_MAX));

    static std::pair<uint32_t, HandleTableChunk *> _get_chunk(const HandleTable &table, Handle handle) {
        auto chunk_index = handle.index / CHUNK_SIZE;
        if (table.pimpl->chunks.find(chunk_index) == table.pimpl->chunks.cend()) {
            return {0, nullptr};
        }

        return {chunk_index, table.pimpl->chunks[chunk_index]};
    }

    static HandleTableEntry *_get_entry(const HandleTable &table, Handle handle) {
        auto *chunk = _get_chunk(table, handle).second;

        if (chunk == nullptr) {
            return nullptr;
        }

        auto index_in_chunk = handle.index % CHUNK_SIZE;

        auto *entry = &chunk->entries[index_in_chunk];

        if (entry->uid != handle.uid) {
            return nullptr;
        }

        return entry;
    }

    HandleTable::HandleTable(void) :
            pimpl(new pimpl_HandleTable()) {
        auto *initial_chunk = new HandleTableChunk();
        for (int64_t i = CHUNK_SIZE - 1; i >= 0; i--) {
            initial_chunk->open_indices.push(uint32_t(i));
        }
        pimpl->chunks.insert({0, initial_chunk});
    }

    HandleTable::~HandleTable(void) {
        delete pimpl;
    }

    Handle HandleTable::create_handle(void *ptr) {
        HandleTableChunk *dest_chunk = nullptr;
        uint32_t dest_chunk_index = 0;
        int64_t last_chunk = -1;
        for (auto chunk_it : pimpl->chunks) {
            auto chunk_index = chunk_it.first;

            auto *chunk = chunk_it.second;
            if (!chunk->open_indices.empty()) {
                dest_chunk = chunk;
                dest_chunk_index = chunk_index;
            }
        }

        if (dest_chunk == nullptr) {
            // need to allocate new chunk

            if (pimpl->chunks.rbegin()->first == MAX_CHUNKS) {
                Logger::default_logger().fatal("Too many handles allocated");
            }

            uint32_t first_empty_chunk_index = MAX_CHUNKS - 1;
            bool found_empty = false;
            for (auto chunk_it : pimpl->chunks) {
                auto chunk_index = chunk_it.first;
                if (chunk_index != last_chunk + 1) {
                    first_empty_chunk_index = uint32_t(last_chunk + 1);
                    found_empty = true;
                    break;
                }
            }
            if (!found_empty) {
                first_empty_chunk_index = pimpl->chunks.rbegin()->first + 1;
            }

            dest_chunk_index = first_empty_chunk_index;

            auto *new_chunk = new HandleTableChunk();
            for (int64_t i = CHUNK_SIZE - 1; i >= 0; i--) {
                new_chunk->open_indices.push(uint32_t(i));
            }

            pimpl->chunks.insert({first_empty_chunk_index, new_chunk});

            dest_chunk = new_chunk;
        }

        auto index_in_chunk = dest_chunk->open_indices.top();
        auto uid = uint32_t(rand_distr(rand_mt));
        dest_chunk->entries[index_in_chunk] = {ptr, uid, 1};
        dest_chunk->open_indices.pop();

        auto handle_index = dest_chunk_index * CHUNK_SIZE + index_in_chunk;
        return {handle_index, uid};
    }

    Handle HandleTable::copy_handle(Handle handle) {
        auto *entry = _get_entry(*this, handle);
        if (entry == nullptr) {
            return handle;
        }

        entry->rc += 1;

        return handle;
    }

    bool HandleTable::update_handle(Handle handle, void *ptr) {
        auto *entry = _get_entry(*this, handle);
        if (entry == nullptr) {
            return false;
        }

        entry->ptr = ptr;
        return true;
    }

    void HandleTable::release_handle(Handle handle) {
        auto chunk_pair = _get_chunk(*this, handle);
        auto chunk_index = chunk_pair.first;
        auto *chunk = chunk_pair.second;

        if (chunk == nullptr) {
            Logger::default_logger().warn("Attempt to free invalid handle");
            return;
        }

        auto entry = &chunk->entries[handle.index % CHUNK_SIZE];
        if (entry->ptr == nullptr) {
            return;
        }

        entry->rc -= 1;
        if (entry->rc > 0) {
            // there are still copies floating around so we can't actually delete it
            return;
        }

        if (chunk->open_indices.size() == CHUNK_SIZE - 1 && pimpl->chunks.size() > 1) {
            // just deallocate the chunk
            delete chunk;
            pimpl->chunks.erase(chunk_index);

            return;
        }

        auto index_in_chunk = handle.index % CHUNK_SIZE;

        chunk->entries[index_in_chunk] = {nullptr, 0, 0};
        chunk->open_indices.push(index_in_chunk);
    }

    void *HandleTable::deref(Handle handle) const {
        auto *entry = _get_entry(*this, handle);
        return entry != nullptr ? entry->ptr : nullptr;
    }
}