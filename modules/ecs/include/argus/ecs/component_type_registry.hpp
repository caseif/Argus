/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
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

            size_t get_component_type_count(void);

            ComponentTypeId get_component_type_id(std::string &type_name);

            size_t get_component_type_size(ComponentTypeId type_id);

            ComponentTypeId register_component(std::string &name, size_t size);

            void _seal(void);

            bool _is_sealed(void);
    };
}
