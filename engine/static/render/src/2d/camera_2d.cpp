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

#include "argus/render/2d/camera_2d.hpp"
#include "argus/render/common/transform.hpp"
#include "internal/render/pimpl/2d/camera_2d.hpp"

#include <string>

namespace argus {
    static PoolAllocator g_alloc_pool(sizeof(pimpl_Camera2D));

    Camera2D::Camera2D(const std::string &id, Scene2D &scene) :
            pimpl(&g_alloc_pool.construct<pimpl_Camera2D>(id, scene)) {
    }

    Camera2D::Camera2D(Camera2D &&rhs) :
            pimpl(rhs.pimpl) {
        rhs.pimpl = nullptr;
    }

    Camera2D::~Camera2D(void) {
        if (pimpl != nullptr) {
            g_alloc_pool.free(pimpl);
        }
    }

    const std::string &Camera2D::get_id(void) const {
        return pimpl->id;
    }

    Scene2D &Camera2D::get_scene(void) const {
        return pimpl->scene;
    }

    Transform2D Camera2D::peek_transform(void) const {
        return pimpl->transform.peek();
    }

    ValueAndDirtyFlag<Transform2D> Camera2D::get_transform(void) {
        return pimpl->transform.read();
    }

    void Camera2D::set_transform(const Transform2D &transform) {
        pimpl->transform = transform;
    }
}
