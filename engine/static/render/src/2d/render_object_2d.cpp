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

#include "argus/render/2d/render_object_2d.hpp"

#include "argus/lowlevel/dirtiable.hpp"
#include "argus/lowlevel/memory.hpp"

#include "argus/render/2d/render_group_2d.hpp"
#include "argus/render/common/transform.hpp"
#include "internal/render/module_render.hpp"
#include "internal/render/pimpl/2d/render_object_2d.hpp"
#include "internal/render/pimpl/common/transform_2d.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace argus {
    // forward declarations
    class RenderPrim2D;

    class Scene2D;

    static PoolAllocator g_pimpl_pool(sizeof(pimpl_RenderObject2D));

    RenderObject2D::RenderObject2D(const RenderGroup2D &parent_group, const std::string &material,
            const std::vector<RenderPrim2D> &primitives, const Vector2f &anchor_point, const Vector2f &atlas_stride,
            uint32_t z_index, float light_opacity, const Transform2D &transform) :
            m_pimpl(&g_pimpl_pool.construct<pimpl_RenderObject2D>(g_render_handle_table.create_handle(this), parent_group,
                    material, primitives, anchor_point, atlas_stride, z_index, light_opacity, transform)) {
        m_pimpl->transform.set_version_ref(m_pimpl->version);
    }

    RenderObject2D::RenderObject2D(Handle handle, const RenderGroup2D &parent_group, const std::string &material,
            const std::vector<RenderPrim2D> &primitives, const Vector2f &anchor_point, const Vector2f &atlas_stride,
            uint32_t z_index, float light_opacity, const Transform2D &transform) :
            m_pimpl(&g_pimpl_pool.construct<pimpl_RenderObject2D>(handle, parent_group,
                    material, primitives, anchor_point, atlas_stride, z_index, light_opacity, transform)) {
        m_pimpl->transform.set_version_ref(m_pimpl->version);
        g_render_handle_table.update_handle(handle, this);
    }

    RenderObject2D::RenderObject2D(RenderObject2D &&rhs) noexcept:
            m_pimpl(rhs.m_pimpl) {
        rhs.m_pimpl = nullptr;
    }

    RenderObject2D::~RenderObject2D() {
        if (m_pimpl != nullptr) {
            g_render_handle_table.release_handle(m_pimpl->handle);
            g_pimpl_pool.destroy(m_pimpl);
        }
    }

    Handle RenderObject2D::get_handle(void) const {
        return m_pimpl->handle;
    }

    const Scene2D &RenderObject2D::get_scene(void) const {
        return m_pimpl->parent_group.get_scene();
    }

    const RenderGroup2D &RenderObject2D::get_parent(void) const {
        return m_pimpl->parent_group;
    }

    const std::string &RenderObject2D::get_material(void) const {
        return m_pimpl->material;
    }

    const std::vector<RenderPrim2D> &RenderObject2D::get_primitives(void) const {
        return m_pimpl->primitives;
    }

    const Vector2f &RenderObject2D::get_anchor_point(void) const {
        return m_pimpl->anchor_point;
    }

    const Vector2f &RenderObject2D::get_atlas_stride(void) const {
        return m_pimpl->atlas_stride;
    }

    uint32_t RenderObject2D::get_z_index(void) const {
        return m_pimpl->z_index;
    }

    float RenderObject2D::get_light_opacity(void) const {
        return m_pimpl->light_opacity.peek();
    }

    void RenderObject2D::set_light_opacity(float opacity) {
        m_pimpl->light_opacity = opacity;
    }

    ValueAndDirtyFlag<Vector2u> RenderObject2D::get_active_frame(void) const {
        return m_pimpl->active_frame.read();
    }

    void RenderObject2D::set_active_frame(const Vector2u &index) {
        m_pimpl->active_frame = index;
    }

    const Transform2D &RenderObject2D::peek_transform(void) const {
        return m_pimpl->transform;
    }

    Transform2D &RenderObject2D::get_transform(void) {
        return m_pimpl->transform;
    }

    void RenderObject2D::set_transform(const Transform2D &transform) {
        m_pimpl->transform = transform;
    }

    RenderObject2D &RenderObject2D::copy(RenderGroup2D &parent) {
        std::vector<RenderPrim2D> prims_copy;
        std::transform(m_pimpl->primitives.begin(), m_pimpl->primitives.end(), std::back_inserter(prims_copy),
                [](auto &v) { return RenderPrim2D(v); });
        auto new_handle = g_render_handle_table.copy_handle(m_pimpl->handle);
        auto &copy = *new RenderObject2D(new_handle, parent, m_pimpl->material, prims_copy, m_pimpl->anchor_point,
                m_pimpl->atlas_stride, m_pimpl->z_index, m_pimpl->light_opacity.peek(), m_pimpl->transform);
        copy.m_pimpl->active_frame = m_pimpl->active_frame;
        copy.m_pimpl->version = m_pimpl->version;

        g_render_handle_table.update_handle(m_pimpl->handle, copy);

        return copy;
    }
}
