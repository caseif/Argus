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

// module core
#include "argus/core/module.hpp"

// module ecs
#include "argus/ecs/component_type_registry.hpp"
#include "internal/ecs/module_ecs.hpp"

#include <string>

namespace argus {
    void update_lifecycle_ecs(LifecycleStage stage) {
        switch (stage) {
            case LifecycleStage::Init: {
                // we only accept component registrations during the pre-init stage
                ComponentTypeRegistry::instance()._seal();
                break;
            }
            default: {
                break;
            }
        }
    }

    void init_module_ecs(void) {
    }
}
