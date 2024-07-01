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
#include "argus/lowlevel/collections.hpp"

#include "argus/core/engine.hpp"

#include "argus/render/common/transform.hpp"
#include "argus/render/2d/render_group_2d.hpp"
#include "argus/render/2d/render_object_2d.hpp"
#include "argus/render/2d/scene_2d.hpp"
#include "internal/render/module_render.hpp"
#include "internal/render/pimpl/2d/render_group_2d.hpp"
#include "internal/render/pimpl/2d/render_object_2d.hpp"
#include "internal/render/pimpl/2d/scene_2d.hpp"

#include <string>
#include <vector>

namespace argus {
    // forward declarations
    class RenderPrim2D;

    class Scene2D;

    static PoolAllocator g_pimpl_pool(sizeof(pimpl_RenderGroup2D));

    static Handle _make_handle(RenderGroup2D *group) {
        return g_render_handle_table.create_handle(group);
    }

    RenderGroup2D::RenderGroup2D(Scene2D &scene, RenderGroup2D *const parent_group,
            const Transform2D &transform):
        m_pimpl(&g_pimpl_pool.construct<pimpl_RenderGroup2D>(_make_handle(this), scene, parent_group, transform)) {
        m_pimpl->transform.set_version_ref(m_pimpl->version);
    }

    RenderGroup2D::RenderGroup2D(Scene2D &scene, RenderGroup2D *const parent_group,
            Transform2D &&transform):
        m_pimpl(&g_pimpl_pool.construct<pimpl_RenderGroup2D>(_make_handle(this), scene, parent_group, transform)) {
        m_pimpl->transform.set_version_ref(m_pimpl->version);
    }

    RenderGroup2D::RenderGroup2D(Scene2D &scene, RenderGroup2D *const parent_group):
        m_pimpl(&g_pimpl_pool.construct<pimpl_RenderGroup2D>(_make_handle(this), scene, parent_group)) {
        m_pimpl->transform.set_version_ref(m_pimpl->version);
    }

    RenderGroup2D::RenderGroup2D(Handle handle, Scene2D &scene, RenderGroup2D *parent_group,
            const Transform2D &transform):
        m_pimpl(&g_pimpl_pool.construct<pimpl_RenderGroup2D>(handle, scene, parent_group, transform)) {
        m_pimpl->transform.set_version_ref(m_pimpl->version);
        g_render_handle_table.update_handle(handle, this);
    }

    RenderGroup2D::RenderGroup2D(RenderGroup2D &&rhs) noexcept:
        m_pimpl(rhs.m_pimpl) {
        rhs.m_pimpl = nullptr;
    }

    RenderGroup2D::~RenderGroup2D(void) {
        if (m_pimpl != nullptr) {
            g_render_handle_table.release_handle(m_pimpl->handle);

            for (auto *group : m_pimpl->child_groups) {
                delete group;
            }

            for (auto *obj : m_pimpl->child_objects) {
                delete obj;
            }

            g_pimpl_pool.destroy(m_pimpl);
        }
    }

    Handle RenderGroup2D::get_handle(void) const {
        return m_pimpl->handle;
    }

    Scene2D &RenderGroup2D::get_scene(void) const {
        return m_pimpl->scene;
    }

    std::optional<std::reference_wrapper<RenderGroup2D>> RenderGroup2D::get_parent(void) const {
        return m_pimpl->parent_group != nullptr
                ? std::make_optional(std::reference_wrapper(*m_pimpl->parent_group))
                : std::nullopt;
    }

    Handle RenderGroup2D::add_group(const Transform2D &transform) {
        auto *group = new RenderGroup2D(m_pimpl->scene, this, transform);
        m_pimpl->child_groups.push_back(group);

        return group->get_handle();
    }

    Handle RenderGroup2D::add_object(const std::string &material,
            const std::vector<RenderPrim2D> &primitives, const Vector2f &anchor_point, const Vector2f &atlas_stride,
            uint32_t z_index, float light_opacity, const Transform2D &transform) {
        auto *obj = new RenderObject2D(*this, material, primitives, anchor_point, atlas_stride, z_index,
                light_opacity, transform);
        m_pimpl->child_objects.push_back(obj);

        return obj->get_handle();
    }

    void RenderGroup2D::remove_group(Handle handle) {
        auto group_opt = m_pimpl->scene.get_group(handle);

        if (!group_opt.has_value()) {
            crash("No group with the given ID exists in the scene");
        }

        auto &group = group_opt.value().get();

        if (group.m_pimpl->parent_group != this) {
            crash("Supplied RenderGroup2D is not a child of RenderGroup2D");
        }

        remove_from_vector(m_pimpl->child_groups, &group);

        delete &group;
    }

    void RenderGroup2D::remove_object(Handle handle) {
        auto object_opt = m_pimpl->scene.get_object(handle);
        if (!object_opt.has_value()) {
            crash("No object with the given ID exists in the scene");
        }

        auto &object = object_opt.value().get();

        /*if (&object.m_pimpl->parent_group != this) {
            throw std::invalid_argument("Supplied RenderObject2D is not a child of RenderGroup2D");
        }*/

        remove_from_vector(m_pimpl->child_objects, &object);

        delete &object;
    }

    const Transform2D &RenderGroup2D::peek_transform(void) const {
        return m_pimpl->transform;
    }

    Transform2D &RenderGroup2D::get_transform(void) {
        return m_pimpl->transform;
    }

    //NOLINTNEXTLINE(readability-make-member-function-const)
    void RenderGroup2D::set_transform(const Transform2D &transform) {
        m_pimpl->transform = transform;
    }

    RenderGroup2D &RenderGroup2D::copy() {
        return copy(nullptr);
    }

    RenderGroup2D &RenderGroup2D::copy(RenderGroup2D *parent) {
        auto new_handle = g_render_handle_table.copy_handle(m_pimpl->handle);
        auto &copy = *new RenderGroup2D(new_handle, m_pimpl->scene, parent, m_pimpl->transform);
        copy.m_pimpl->version = m_pimpl->version;

        copy.m_pimpl->child_groups.reserve(m_pimpl->child_groups.size());
        for (auto &child_group : m_pimpl->child_groups) {
            auto &child_copy = child_group->copy(this);
            child_copy.m_pimpl->parent_group = this;
            copy.m_pimpl->child_groups.push_back(&child_copy);
        }

        copy.m_pimpl->child_objects.reserve(m_pimpl->child_objects.size());
        for (auto *child_obj : m_pimpl->child_objects) {
            auto &child_copy = child_obj->copy(*this);
            copy.m_pimpl->child_objects.push_back(&child_copy);
        }

        return copy;
    }
}
