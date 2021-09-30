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

// module resman
#include "argus/resman/resource_manager.hpp"
#include "internal/resman/module_resman.hpp"

#include <string>

namespace argus {
    void update_lifecycle_resman(LifecycleStage stage) {
        switch (stage) {
            case LifecycleStage::PostInit:
                ResourceManager::get_global_resource_manager().discover_resources();
                break;
            case LifecycleStage::PostDeinit:
                // not necessary for now since it's static
                //ResourceManager::get_global_resource_manager().~ResourceManager();
                break;
            default:
                break;
        }
    }

    void init_module_resman(void) {
    }
}