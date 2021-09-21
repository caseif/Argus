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

// module lowlevel
#include "argus/lowlevel/memory.hpp"

// module render
#include "argus/render/common/transform.hpp"
#include "argus/render/2d/render_group_2d.hpp"
#include "argus/render/2d/render_object_2d.hpp"
#include "internal/render/pimpl/common/transform_2d.hpp"
#include "internal/render/pimpl/2d/render_object_2d.hpp"

#include <atomic>
#include <vector>
#include <string>

namespace argus {
    // forward declarations
    class RenderPrim2D;
    class Scene2D;

    static AllocPool g_pimpl_pool(sizeof(pimpl_RenderObject2D));

    RenderObject2D::RenderObject2D(const RenderGroup2D &parent_group, const std::string &material,
            const std::vector<RenderPrim2D> &primitives, const Transform2D &transform):
        pimpl(&g_pimpl_pool.construct<pimpl_RenderObject2D>(parent_group, material, primitives, transform)) {
    }

    RenderObject2D::RenderObject2D(const RenderObject2D &rhs) noexcept:
        pimpl(&g_pimpl_pool.construct<pimpl_RenderObject2D>(*rhs.pimpl)) {
    }

    RenderObject2D::RenderObject2D(RenderObject2D &&rhs) noexcept:
        pimpl(rhs.pimpl) {
        rhs.pimpl = nullptr;
    }

    RenderObject2D::~RenderObject2D() {
        if (pimpl != nullptr) {
            g_pimpl_pool.destroy(pimpl);
        }
    }

    const Scene2D &RenderObject2D::get_scene(void) const {
        return pimpl->parent_group.get_scene();
    }

    std::string RenderObject2D::get_material(void) const {
        return pimpl->material;
    }

    const std::vector<RenderPrim2D> &RenderObject2D::get_primitives(void) const {
        return pimpl->primitives;
    }

    Transform2D &RenderObject2D::get_transform(void) const {
        return pimpl->transform;
    }

    void RenderObject2D::set_transform(Transform2D &transform) const {
        pimpl->transform = transform;
        pimpl->transform.pimpl->dirty = true;
    }
}
