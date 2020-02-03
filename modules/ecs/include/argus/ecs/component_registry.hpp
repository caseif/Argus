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
    typedef uint16_t ComponentId;

    struct pimpl_ComponentRegistry;

    class ComponentRegistry {
        private:
            pimpl_ComponentRegistry *pimpl;

            ComponentRegistry(void);

        public:
            static ComponentRegistry &instance(void);

            ComponentId get_component_id(std::string &component_name);

            size_t get_component_size(ComponentId component_id);

            ComponentId register_component(std::string &name, size_t size);

            void _seal(void);

            bool _is_sealed(void);
    };
}
