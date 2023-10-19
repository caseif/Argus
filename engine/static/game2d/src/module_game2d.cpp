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

#include "argus/lowlevel/handle.hpp"

#include "argus/core/engine.hpp"
#include "argus/core/module.hpp"

#include "argus/resman/resource_manager.hpp"

#include "internal/game2d/module_game2d.hpp"
#include "internal/game2d/resources.h"
#include "internal/game2d/script_bindings.hpp"
#include "internal/game2d/world2d.hpp"
#include "internal/game2d/loader/sprite_loader.hpp"

namespace argus {
    HandleTable g_static_obj_table;
    HandleTable g_actor_table;

    void update_lifecycle_game2d(LifecycleStage stage) {
        switch (stage) {
            case LifecycleStage::Init: {
                ResourceManager::instance().register_loader(*new SpriteLoader());

                register_update_callback(render_worlds);

                register_game2d_bindings();

                break;
            }
            case LifecycleStage::PostInit: {
                ResourceManager::instance().add_memory_package(RESOURCES_GAME2D_ARP_SRC,
                        RESOURCES_GAME2D_ARP_LEN);
                break;
            }
            case LifecycleStage::PreDeinit: {
                break;
            }
            default: {
                break;
            }
        }
    }
}
