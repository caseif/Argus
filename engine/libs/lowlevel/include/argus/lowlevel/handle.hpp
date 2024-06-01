/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2024, Max Roncace <mproncace@protonmail.com>
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

#include <type_traits>

#include <cstdint>

namespace argus {
    struct pimpl_HandleTable;

    struct Handle {
        uint32_t index;
        uint32_t uid;

        bool operator==(const Handle &rhs) const;

        bool operator<(const Handle &rhs) const;

        operator uint64_t(void) const;
    };

    class HandleTable {
      public:
        pimpl_HandleTable *m_pimpl;

        explicit HandleTable(void);

        HandleTable(const HandleTable &) = delete;

        HandleTable(HandleTable &&) = delete;

        ~HandleTable(void);

        Handle create_handle(void *ptr);

        template<typename T>
        Handle create_handle(T *ref) {
            return create_handle(static_cast<void *>(ref));
        }

        template<typename T, typename = typename std::enable_if<!std::is_pointer<T>::value, T>::type>
        Handle create_handle(T &ref) {
            return create_handle(static_cast<void *>(&ref));
        }

        Handle copy_handle(Handle handle);

        bool update_handle(Handle handle, void *ptr);

        template<typename T>
        bool update_handle(Handle handle, T *ref) {
            return update_handle(handle, static_cast<void *>(ref));
        }

        template<typename T, typename = typename std::enable_if<!std::is_pointer<T>::value, T>::type>
        bool update_handle(Handle handle, T &ref) {
            return update_handle(handle, static_cast<void *>(&ref));
        }

        void release_handle(Handle handle);

        [[nodiscard]] void *deref(Handle handle) const;

        template<typename T>
        T *deref(Handle handle) const {
            return reinterpret_cast<T *>(deref(handle));
        }
    };
}
