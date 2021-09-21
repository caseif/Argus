/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@protonmail.com>
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

#include <cstdint>

namespace argus {
    typedef void *ComponentHandle;
    typedef uint16_t ComponentTypeId;

    struct pimpl_ComponentTypeRegistry;

    class ComponentTypeRegistry {
        private:
            pimpl_ComponentTypeRegistry *pimpl;

            ComponentTypeRegistry(void);

        public:
            static ComponentTypeRegistry &instance(void);

            void *alloc_component(ComponentTypeId type_id);

            void free_component(ComponentTypeId type_id, void *ptr);

            size_t get_component_type_count(void);

            ComponentTypeId get_component_type_id(std::string &type_name);

            size_t get_component_type_size(ComponentTypeId type_id);

            ComponentTypeId register_component_type(std::string &name, size_t size);

            void _seal(void);

            bool _is_sealed(void);
    };
}
