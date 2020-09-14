/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

#include "argus/ecs/entity.hpp"

namespace argus {

// disable non-standard extension warning for flexible array member
#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable : 4200)
#endif
    struct pimpl_Entity {
        const EntityId id;
        void *component_pointers[0];
    };
#ifdef _MSC_VER
    #pragma warning(pop)
#endif

}
