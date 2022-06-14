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

#include "argus/lowlevel/macros.hpp"

#include "argus/ecs/entity.hpp"
#include "argus/ecs/system.hpp"
#include "argus/ecs/system_builder.hpp"
#include "argus/game2d/components.hpp"

#include <chrono>
#include <string>

namespace argus {
    static constexpr const char *k_system_name = "render_background_layers";

    static void _process_entity(const Entity &entity, TimeDelta delta) {
        UNUSED(entity);
        UNUSED(delta);
        //TODO
    }

    System &create_textured_background_system(void) {
        return System::builder()
            .with_name(k_system_name)
            .with_callback(_process_entity)
            .targets<BackgroundComponent>()
            .targets<IndexComponent>()
            .targets<PositionComponent>()
            .targets<TextureComponent>()
            .targets<WorldComponent>()
            .build();
    }
}
