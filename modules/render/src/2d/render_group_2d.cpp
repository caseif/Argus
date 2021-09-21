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

// module core
#include "internal/core/core_util.hpp"

// module render
#include "argus/render/common/transform.hpp"
#include "argus/render/2d/render_group_2d.hpp"
#include "argus/render/2d/render_object_2d.hpp"
#include "internal/render/pimpl/2d/render_group_2d.hpp"
#include "internal/render/pimpl/2d/render_object_2d.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

namespace argus {
    // forward declarations
    class RenderPrim2D;
    class Scene2D;

    static AllocPool g_pimpl_pool(sizeof(pimpl_RenderGroup2D));

    RenderGroup2D::RenderGroup2D(Scene2D &scene, RenderGroup2D *const parent_group,
            const Transform2D &transform):
        pimpl(&g_pimpl_pool.construct<pimpl_RenderGroup2D>(scene, parent_group, transform)) {
    }

    RenderGroup2D::RenderGroup2D(Scene2D &scene, RenderGroup2D *const parent_group,
            Transform2D &&transform):
        pimpl(&g_pimpl_pool.construct<pimpl_RenderGroup2D>(scene, parent_group, transform)) {
    }

    RenderGroup2D::RenderGroup2D(Scene2D &scene, RenderGroup2D *const parent_group):
        pimpl(&g_pimpl_pool.construct<pimpl_RenderGroup2D>(scene, parent_group)) {
    }

    RenderGroup2D::RenderGroup2D(const RenderGroup2D &rhs) noexcept:
        pimpl(&g_pimpl_pool.construct<pimpl_RenderGroup2D>(*rhs.pimpl)) {
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

    Scene2D &RenderGroup2D::get_scene(void) const {
        return pimpl->scene;
    }

    RenderGroup2D *RenderGroup2D::get_parent_group(void) const {
        return pimpl->parent_group;
    }

    RenderGroup2D &RenderGroup2D::create_child_group(const Transform2D &transform) {
        auto *group = new RenderGroup2D(pimpl->scene, this, transform);
        pimpl->child_groups.push_back(group);
        return *group;
    }

    RenderObject2D &RenderGroup2D::create_child_object(const std::string &material,
            const std::vector<RenderPrim2D> &primitives, const Transform2D &transform) {
        auto *obj = new RenderObject2D(*this, material, primitives, transform);
        pimpl->child_objects.push_back(obj);
        return *obj;
    }

    void RenderGroup2D::remove_member_group(RenderGroup2D &group) {
        if (group.pimpl->parent_group != this) {
            throw std::invalid_argument("Supplied RenderGroup2D is not a child of RenderGroup2D");
        }

        remove_from_vector(pimpl->child_groups, &group);
    }

    void RenderGroup2D::remove_child_object(RenderObject2D &object) {
        if (&object.pimpl->parent_group != this) {
            throw std::invalid_argument("Supplied RenderObject2D is not a child of RenderGroup2D");
        }

        remove_from_vector(pimpl->child_objects, &object);

        delete &object;
    }

    Transform2D &RenderGroup2D::get_transform(void) const {
        return pimpl->transform;
    }

    //NOLINTNEXTLINE(readability-make-member-function-const)
    void RenderGroup2D::set_transform(const Transform2D &transform) {
        pimpl->transform = transform;
    }
}
