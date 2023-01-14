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

#include "argus/core/engine.hpp"
#include "argus/core/module.hpp"

#include "argus/ecs/component_type_registry.hpp"
#include "internal/ecs/module_ecs.hpp"
#include "internal/ecs/system_executor.hpp"

#include <string>

namespace argus {
    bool g_ecs_initialized = false;

    void update_lifecycle_ecs(LifecycleStage stage) {
        switch (stage) {
            case LifecycleStage::PostInit: {
                // we only accept component and system registrations during the pre-init and init stages
                ComponentTypeRegistry::instance()._seal();

                g_ecs_initialized = true;

                register_update_callback(execute_all_systems);

                break;
            }
            default: {
                break;
            }
        }
    }
}
