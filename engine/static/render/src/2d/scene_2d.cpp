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

#include "argus/render/common/scene.hpp"
#include "argus/render/2d/render_group_2d.hpp"
#include "argus/render/2d/render_object_2d.hpp"
#include "argus/render/2d/scene_2d.hpp"
#include "internal/render/pimpl/2d/render_object_2d.hpp"
#include "internal/render/pimpl/2d/scene_2d.hpp"

#include <stdexcept>
#include <string>
#include <vector>

namespace argus {
    // forward declarations
    class Canvas;

    class RenderPrim2D;
    class Transform2D;

    struct pimpl_Scene;

    static AllocPool g_pimpl_pool(sizeof(pimpl_Scene2D));

    Scene2D::Scene2D(const Canvas &canvas, const Transform2D &transform, const int index):
        Scene(SceneType::TwoD),
        pimpl(&g_pimpl_pool.construct<pimpl_Scene2D>(canvas, *this, transform, index)) {
    }

    Scene2D::Scene2D(const Scene2D &rhs) noexcept:
        Scene(SceneType::TwoD),
        pimpl(&g_pimpl_pool.construct<pimpl_Scene2D>(*rhs.pimpl)) {
    }

    Scene2D::Scene2D(Scene2D &&rhs) noexcept:
        Scene(SceneType::TwoD),
        pimpl(rhs.pimpl) {
        rhs.pimpl = nullptr;
    }

    Scene2D::~Scene2D(void) {
        if (pimpl != nullptr) {
            g_pimpl_pool.destroy(pimpl);
        }
    }

    pimpl_Scene *Scene2D::get_pimpl(void) const {
        return dynamic_cast<pimpl_Scene*>(pimpl);
    }

    RenderGroup2D &Scene2D::create_child_group(const Transform2D &transform) {
        return pimpl->root_group.create_child_group(transform);
    }

    RenderObject2D &Scene2D::create_child_object(const std::string &material,
            const std::vector<RenderPrim2D> &primitives, const Transform2D &transform) {
        return pimpl->root_group.create_child_object(material, primitives, transform);
    }

    void Scene2D::remove_member_group(RenderGroup2D &group) {
        if (group.get_parent_group() != &pimpl->root_group) {
            throw std::invalid_argument("Supplied RenderGroup2D is not a direct child of the Scene2D");
        }

        pimpl->root_group.remove_member_group(group);
    }

    void Scene2D::remove_member_object(RenderObject2D &object) {
        if (&object.pimpl->parent_group != &pimpl->root_group) {
            throw std::invalid_argument("Supplied RenderObject2D is not a direct child of the Scene2D");
        }

        pimpl->root_group.remove_child_object(object);
    }
}
