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

#include "argus/lowlevel/memory.hpp"

#include "argus/game2d/sprite.hpp"
#include "internal/game2d/pimpl/sprite.hpp"

namespace argus {
    static AllocPool g_pimpl_pool(sizeof(pimpl_Sprite));

    Sprite::Sprite(const std::string &id, const Vector2f &base_size, const std::string &texture_uid,
            const std::pair<Vector2f, Vector2f> tex_coords):
            pimpl(&g_pimpl_pool.construct<pimpl_Sprite>(id, base_size, texture_uid, tex_coords)) {
    }

    Sprite::Sprite(Sprite &&rhs):
            pimpl(rhs.pimpl) {
        rhs.pimpl = nullptr;
    }

    Sprite::~Sprite(void) {
        g_pimpl_pool.destroy(pimpl);
    }

    const std::string &Sprite::get_id(void) const {
        return pimpl->id;
    }

    const Vector2f &Sprite::get_base_size(void) const {
        return pimpl->base_size;
    }

    const std::string &Sprite::get_texture(void) const {
        return pimpl->texture_uid;
    }

    const std::pair<Vector2f, Vector2f> &Sprite::get_texture_coords(void) const {
        return pimpl->tex_coords;
    }

    const Transform2D &Sprite::get_transform(void) const {
        return pimpl->transform;
    }

    void Sprite::set_transform(const Transform2D &transform) {
        pimpl->transform = transform;
    }
}
