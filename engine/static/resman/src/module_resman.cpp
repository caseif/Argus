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

#include "argus/lowlevel/logging.hpp"

#include "argus/core/module.hpp"

#include "argus/resman/resource_manager.hpp"
#include "internal/resman/module_resman.hpp"

#include <string>

namespace argus {
    void update_lifecycle_resman(LifecycleStage stage) {
        switch (stage) {
            case LifecycleStage::PostInit:
                Logger::default_logger().debug("Discovering resources");
                ResourceManager::instance().discover_resources();
                break;
            case LifecycleStage::PostDeinit:
                // not necessary for now since it's static
                //ResourceManager::instance().~ResourceManager();
                break;
            default:
                break;
        }
    }
}
