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

#include "argus/lowlevel/memory.hpp"
#include "argus/lowlevel/uuid.hpp"

#include "argus/resman/resource_manager.hpp"

#include "argus/render/common/transform.hpp"

#include "argus/game2d/actor_2d.hpp"
#include "internal/game2d/pimpl/actor_2d.hpp"

namespace argus {
    static AllocPool g_pimpl_pool(sizeof(pimpl_Actor2D));

    Actor2D::Actor2D(const std::string &sprite_uid, const Vector2f &size, const Transform2D &transform) {
        auto &res = ResourceManager::instance().get_resource(sprite_uid);
        auto *sprite = new Sprite(res);

        auto uuid = Uuid::random();
        pimpl = &g_pimpl_pool.construct<pimpl_Actor2D>(uuid, size, transform, res, *sprite);
    }

    Actor2D::~Actor2D(void) {
        pimpl->sprite_def_res.release();
        delete &pimpl->sprite;

        g_pimpl_pool.destroy<pimpl_Actor2D>(pimpl);
    }

    const Uuid &Actor2D::get_uuid(void) const {
        return pimpl->uuid;
    }

    const Vector2f &Actor2D::get_size(void) const {
        return pimpl->size;
    }

    const Transform2D &Actor2D::get_transform(void) const {
        return pimpl->transform.peek();
    }

    void Actor2D::set_transform(const Transform2D &transform) {
        pimpl->transform = transform;
    }

    Sprite &Actor2D::get_sprite(void) const {
        return pimpl->sprite;
    }
}
