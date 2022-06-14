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

#include "argus/lowlevel/math.hpp"

#include "argus/ecs/entity.hpp"
#include "argus/ecs/entity_builder.hpp"

#include "argus/game2d/components.hpp"
#include "argus/game2d/world2d.hpp"

#include <string>

#include <cstdint>

namespace argus {
    Entity &World2D::create_textured_background_layer(const std::string &id, uint8_t index,
            const std::string &texture_uid, const Vector2f &base_offset, const Vector2f &parallax_coeff) {
        auto &entity = Entity::builder()
            .with<BackgroundComponent>({ .base_offset = base_offset, .parallax_coeff = parallax_coeff })
            .with<IdentifierComponent>({ .id = id })
            .with<IndexComponent>({ .index = index })
            .with<PositionComponent>()
            .with<TextureComponent>({ .uid = texture_uid })
            .with<WorldComponent>({ .world = *this })
            .build();
        return entity;
    }
}
