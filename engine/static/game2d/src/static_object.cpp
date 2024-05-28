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

#include "argus/lowlevel/math.hpp"
#include "argus/lowlevel/memory.hpp"

#include "argus/resman/resource_manager.hpp"

#include "argus/render/common/transform.hpp"

#include "argus/game2d/static_object_2d.hpp"
#include "internal/game2d/pimpl/static_object_2d.hpp"
#include "internal/game2d/module_game2d.hpp"

#include <string>

namespace argus {
    static PoolAllocator g_pimpl_pool(sizeof(pimpl_StaticObject2D));

    StaticObject2D::StaticObject2D(const std::string &sprite_uid, const Vector2f &size, uint32_t z_index,
            bool can_occlude_light, const Transform2D &transform) {
        auto &res = ResourceManager::instance().get_resource(sprite_uid);
        auto *sprite = new Sprite(res);

        auto handle = g_static_obj_table.create_handle(this);

        pimpl = &g_pimpl_pool.construct<pimpl_StaticObject2D>(handle, res, *sprite, size, z_index,
                can_occlude_light, transform);
    }

    StaticObject2D::StaticObject2D(StaticObject2D &&rhs) noexcept:
            pimpl(rhs.pimpl) {
        rhs.pimpl = nullptr;
    }

    StaticObject2D::~StaticObject2D(void) {
        if (pimpl != nullptr) {
            g_static_obj_table.release_handle(pimpl->handle);
            g_pimpl_pool.free(pimpl);
        }
    }

    Vector2f StaticObject2D::get_size(void) const {
        return pimpl->size;
    }

    uint32_t StaticObject2D::get_z_index(void) const {
        return pimpl->z_index;
    }

    bool StaticObject2D::can_occlude_light(void) const {
        return pimpl->can_occlude_light;
    }

    const Transform2D &StaticObject2D::get_transform(void) const {
        return pimpl->transform;
    }

    Sprite &StaticObject2D::get_sprite(void) const {
        return pimpl->sprite;
    }
}
