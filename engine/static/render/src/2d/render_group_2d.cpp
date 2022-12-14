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
#include "argus/lowlevel/vector.hpp"

#include "argus/render/common/transform.hpp"
#include "argus/render/2d/render_group_2d.hpp"
#include "argus/render/2d/render_object_2d.hpp"
#include "argus/render/2d/scene_2d.hpp"
#include "internal/render/pimpl/2d/render_group_2d.hpp"
#include "internal/render/pimpl/2d/render_object_2d.hpp"
#include "internal/render/pimpl/2d/scene_2d.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

namespace argus {
    // forward declarations
    class RenderPrim2D;
    class Scene2D;

    static AllocPool g_pimpl_pool(sizeof(pimpl_RenderGroup2D));

    RenderGroup2D::RenderGroup2D(const std::string &id, Scene2D &scene, RenderGroup2D *const parent_group,
            const Transform2D &transform):
        pimpl(&g_pimpl_pool.construct<pimpl_RenderGroup2D>(id, scene, parent_group, transform)) {
    }

    RenderGroup2D::RenderGroup2D(const std::string &id, Scene2D &scene, RenderGroup2D *const parent_group,
            Transform2D &&transform):
        pimpl(&g_pimpl_pool.construct<pimpl_RenderGroup2D>(id, scene, parent_group, transform)) {
    }

    RenderGroup2D::RenderGroup2D(const std::string &id, Scene2D &scene, RenderGroup2D *const parent_group):
        pimpl(&g_pimpl_pool.construct<pimpl_RenderGroup2D>(id, scene, parent_group)) {
    }

    RenderGroup2D::RenderGroup2D(RenderGroup2D &&rhs) noexcept:
        pimpl(rhs.pimpl) {
        rhs.pimpl = nullptr;
    }

    RenderGroup2D::~RenderGroup2D(void) {
        if (pimpl != nullptr) {
            for (auto *group : pimpl->child_groups) {
                delete group;
            }

            for (auto *obj : pimpl->child_objects) {
                delete obj;
            }

            g_pimpl_pool.destroy(pimpl);
        }
    }

    const std::string &RenderGroup2D::get_id(void) const {
        return pimpl->id;
    }

    Scene2D &RenderGroup2D::get_scene(void) const {
        return pimpl->scene;
    }

    RenderGroup2D *RenderGroup2D::get_parent_group(void) const {
        return pimpl->parent_group;
    }

    RenderGroup2D &RenderGroup2D::create_child_group(const std::string &id, const Transform2D &transform) {
        if (pimpl->scene.get_group(id).has_value()) {
            throw std::invalid_argument("Render group ID must be unique within scene");
        }

        auto *group = new RenderGroup2D(id, pimpl->scene, this, transform);
        pimpl->child_groups.push_back(group);

        pimpl->scene.pimpl->group_map.insert({ group->get_id(), group });

        return *group;
    }

    RenderObject2D &RenderGroup2D::create_child_object(const std::string &id,
            const std::string &material, const std::vector<RenderPrim2D> &primitives,
            const Vector2f &atlas_stride, const Transform2D &transform) {
        if (pimpl->scene.get_group(id).has_value()) {
            throw std::invalid_argument("Render group ID must be unique within scene");
        }

        auto *obj = new RenderObject2D(id, *this, material, primitives, atlas_stride, transform);
        pimpl->child_objects.push_back(obj);

        pimpl->scene.pimpl->object_map.insert({ obj->get_id(), obj });

        return *obj;
    }

    void RenderGroup2D::remove_member_group(const std::string &id) {
        auto group_opt = pimpl->scene.get_group(id);

        if (!group_opt.has_value()) {
            throw std::invalid_argument("No group with the given ID exists in the scene");
        }

        auto &group = group_opt.value().get();

        if (group.pimpl->parent_group != this) {
            throw std::invalid_argument("Supplied RenderGroup2D is not a child of RenderGroup2D");
        }

        remove_from_vector(pimpl->child_groups, &group);

        pimpl->scene.pimpl->group_map.erase(group.get_id());

        delete &group;
    }

    void RenderGroup2D::remove_child_object(const std::string &id) {
        auto object_opt = pimpl->scene.get_object(id);
        if (!object_opt.has_value()) {
            throw std::invalid_argument("No object with the given ID exists in the scene");
        }

        auto &object = object_opt.value().get();

        if (&object.pimpl->parent_group != this) {
            throw std::invalid_argument("Supplied RenderObject2D is not a child of RenderGroup2D");
        }

        remove_from_vector(pimpl->child_objects, &object);

        pimpl->scene.pimpl->object_map.erase(object.get_id());

        delete &object;
    }

    const Transform2D &RenderGroup2D::peek_transform(void) const {
        return pimpl->transform;
    }

    ValueAndDirtyFlag<Transform2D> RenderGroup2D::get_transform(void) {
        return pimpl->transform;
    }

    //NOLINTNEXTLINE(readability-make-member-function-const)
    void RenderGroup2D::set_transform(const Transform2D &transform) {
        pimpl->transform = transform;
    }

    RenderGroup2D &RenderGroup2D::copy() {
        return copy(nullptr);
    }

    RenderGroup2D &RenderGroup2D::copy(RenderGroup2D *parent) {
        auto &copy = *new RenderGroup2D(pimpl->id, pimpl->scene, parent, pimpl->transform);

        copy.pimpl->child_groups.reserve(pimpl->child_groups.size());
        for (auto &child_group : pimpl->child_groups) {
            auto &child_copy = child_group->copy(this);
            child_copy.pimpl->parent_group = this;
            copy.pimpl->child_groups.push_back(&child_copy);
            pimpl->scene.pimpl->group_map.insert({ child_copy.get_id(), &child_copy });
        }

        copy.pimpl->child_objects.reserve(pimpl->child_objects.size());
        for (auto *child_obj : pimpl->child_objects) {
            auto &child_copy = child_obj->copy(*this);
            copy.pimpl->child_objects.push_back(&child_copy);
            pimpl->scene.pimpl->object_map.insert({ child_copy.get_id(), &child_copy });
        }

        return copy;
    }
}
