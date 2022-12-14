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

#include "argus/render/2d/render_object_2d.hpp"

#include "argus/lowlevel/dirtiable.hpp"
#include "argus/lowlevel/memory.hpp"
#include "argus/lowlevel/uuid.hpp"

#include "argus/render/2d/render_group_2d.hpp"
#include "argus/render/common/transform.hpp"
#include "internal/render/pimpl/2d/render_object_2d.hpp"
#include "internal/render/pimpl/common/transform_2d.hpp"

#include <algorithm>
#include <atomic>
#include <string>
#include <vector>

namespace argus {
    // forward declarations
    class RenderPrim2D;
    class Scene2D;

    static AllocPool g_pimpl_pool(sizeof(pimpl_RenderObject2D));

    RenderObject2D::RenderObject2D(const std::string &id, const RenderGroup2D &parent_group,
            const std::string &material, const std::vector<RenderPrim2D> &primitives, const Vector2f &atlas_stride,
            const Transform2D &transform) {
        pimpl = &g_pimpl_pool.construct<pimpl_RenderObject2D>(id, parent_group, material, primitives, atlas_stride,
                transform);
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

    const std::string &RenderObject2D::get_id(void) const {
        return pimpl->id;
    }

    const Scene2D &RenderObject2D::get_scene(void) const {
        return pimpl->parent_group.get_scene();
    }

    const std::string &RenderObject2D::get_material(void) const {
        return pimpl->material;
    }

    const std::vector<RenderPrim2D> &RenderObject2D::get_primitives(void) const {
        return pimpl->primitives;
    }

    const Vector2f &RenderObject2D::get_atlas_stride(void) const {
        return pimpl->atlas_stride;
    }

    void RenderObject2D::set_active_frame(const Vector2u &index) const {
        pimpl->active_frame = index;
        pimpl->active_frame_dirty = true;
    }

    const Transform2D &RenderObject2D::peek_transform(void) const {
        return pimpl->transform;
    }

    ValueAndDirtyFlag<Transform2D> RenderObject2D::get_transform(void) {
        return pimpl->transform;
    }

    void RenderObject2D::set_transform(const Transform2D &transform) const {
        pimpl->transform = transform;
        pimpl->transform.pimpl->dirty = true;
    }

    RenderObject2D &RenderObject2D::copy(RenderGroup2D &parent) {
        std::vector<RenderPrim2D> prims_copy;
        std::transform(pimpl->primitives.begin(), pimpl->primitives.end(), std::back_inserter(prims_copy),
               [] (auto &v) { return RenderPrim2D(v); });
        auto *copy = new RenderObject2D(pimpl->id, parent, pimpl->material, prims_copy, pimpl->atlas_stride,
                pimpl->transform);
        copy->pimpl->active_frame = pimpl->active_frame;
        copy->pimpl->active_frame_dirty = pimpl->active_frame_dirty;
        return *copy;
    }
}
