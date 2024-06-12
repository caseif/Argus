/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2024, Max Roncace <mproncace@protonmail.com>
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
#include "internal/game2d/module_game2d.hpp"
#include "internal/game2d/pimpl/actor_2d.hpp"

namespace argus {
    static PoolAllocator g_pimpl_pool(sizeof(pimpl_Actor2D));

    Actor2D::Actor2D(const std::string &sprite_uid, const Vector2f &size, uint32_t z_index,
            bool can_occlude_light, const Transform2D &transform) {
        auto &res = ResourceManager::instance().get_resource(sprite_uid).expect("Failed to load sprite '"
                + sprite_uid + "' for Actor2D");
        auto *sprite = new Sprite(res);

        auto handle = g_actor_table.create_handle(this);
        m_pimpl = &g_pimpl_pool.construct<pimpl_Actor2D>(handle, size, z_index, can_occlude_light,
                transform, res, *sprite);
    }

    Actor2D::~Actor2D(void) {
        g_actor_table.release_handle(m_pimpl->handle);

        m_pimpl->sprite_def_res.release();
        delete &m_pimpl->sprite;

        g_pimpl_pool.destroy<pimpl_Actor2D>(m_pimpl);
    }

    Vector2f Actor2D::get_size(void) const {
        return m_pimpl->size;
    }

    uint32_t Actor2D::get_z_index(void) const {
        return m_pimpl->z_index;
    }

    bool Actor2D::can_occlude_light(void) const {
        return m_pimpl->can_occlude_light.peek();
    }

    void Actor2D::set_can_occlude_light(bool can_occlude) {
        m_pimpl->can_occlude_light = can_occlude;
    }

    const Transform2D &Actor2D::get_transform(void) const {
        return m_pimpl->transform.peek();
    }

    void Actor2D::set_transform(const Transform2D &transform) {
        m_pimpl->transform = transform;
    }

    Sprite &Actor2D::get_sprite(void) const {
        return m_pimpl->sprite;
    }
}
