/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2022, Max Roncace <mproncace@protonmail.com>
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

#include <string>
#include <typeindex>
#include <typeinfo>
#include <type_traits>

#include <cstdint>

namespace argus {
    typedef uint16_t ComponentTypeId;

    struct pimpl_ComponentTypeRegistry;

    class ComponentTypeRegistry {
        private:
            pimpl_ComponentTypeRegistry *pimpl;

            ComponentTypeRegistry(void);

        public:
            static ComponentTypeRegistry &instance(void);

            void *alloc(std::type_index type);

            template <typename T>
            T &alloc(void) {
                return *static_cast<T*>(alloc(std::type_index(typeid(T))));
            }

            void free(std::type_index type, void *ptr);

            void free(ComponentTypeId id, void *ptr);

            template <typename T>
            void free(void *ptr) {
                free(std::type_index(typeid(T)), ptr);
            }

            size_t get_type_count(void);

            ComponentTypeId get_id(std::type_index type);
            
            template <typename T>
            ComponentTypeId get_id(void) {
                return get_id(std::type_index(typeid(T)));
            }

            void register_type(std::type_index type, size_t size);

            template <typename T>
            void register_type(void) {
                register_type(std::type_index(typeid(T)), sizeof(T));
            }

            void _seal(void);

            bool _is_sealed(void);
    };
}
