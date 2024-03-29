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

#include "argus/lowlevel/color.hpp"
#include "argus/lowlevel/memory.hpp"

#include "argus/render/2d/camera_2d.hpp"
#include "argus/render/common/scene.hpp"
#include "argus/render/2d/render_group_2d.hpp"
#include "argus/render/2d/render_object_2d.hpp"
#include "argus/render/2d/scene_2d.hpp"
#include "internal/render/common/scene.hpp"
#include "internal/render/pimpl/2d/render_object_2d.hpp"
#include "internal/render/pimpl/2d/scene_2d.hpp"
#include "internal/render/module_render.hpp"

#include <stdexcept>
#include <string>
#include <vector>

namespace argus {
    // forward declarations
    class Canvas;

    class RenderPrim2D;

    class Transform2D;

    struct pimpl_Scene;

    static PoolAllocator g_pimpl_pool(sizeof(pimpl_Scene2D));

    Scene2D &Scene2D::create(const std::string &id) {
        if (g_scenes.find(id) != g_scenes.end()) {
            throw std::invalid_argument("Scene with given ID already exists");
        }

        auto scene = new Scene2D(id, Transform2D());
        g_scenes.insert({id, scene});

        return *scene;
    }

    Scene2D::Scene2D(const std::string &id, const Transform2D &transform) :
            Scene(SceneType::TwoD),
            pimpl(&g_pimpl_pool.construct<pimpl_Scene2D>(id, *this, transform)) {
    }

    Scene2D::Scene2D(Scene2D &&rhs) noexcept:
            Scene(SceneType::TwoD),
            pimpl(rhs.pimpl) {
        rhs.pimpl = nullptr;
    }

    Scene2D::~Scene2D(void) {
        g_scenes.erase(g_scenes.find(pimpl->id), g_scenes.end());

        if (pimpl != nullptr) {
            g_pimpl_pool.destroy(pimpl);
        }
    }

    pimpl_Scene *Scene2D::get_pimpl(void) const {
        return dynamic_cast<pimpl_Scene *>(pimpl);
    }

    float Scene2D::peek_ambient_light_level(void) const {
        return pimpl->ambient_light_level.peek();
    }

    ValueAndDirtyFlag<float> Scene2D::get_ambient_light_level(void) {
        return pimpl->ambient_light_level.read();
    }

    void Scene2D::set_ambient_light_level(float level) {
        pimpl->ambient_light_level = level;
    }

    const Vector3f &Scene2D::peek_ambient_light_color(void) const {
        return pimpl->ambient_light_color.peek();
    }

    ValueAndDirtyFlag<Vector3f> Scene2D::get_ambient_light_color(void) {
        return pimpl->ambient_light_color.read();
    }

    void Scene2D::set_ambient_light_color(const Vector3f &color) {
        // normalize the RGB value
        auto hsv_color = rgb_to_hsv(color);
        hsv_color.z = 1.0; // set value to max
        auto final_rgb = hsv_to_rgb(hsv_color);

        pimpl->ambient_light_color = final_rgb;
    }

    std::optional<std::reference_wrapper<RenderGroup2D>> Scene2D::get_group(Handle handle) {
        auto *ptr = g_render_handle_table.deref<RenderGroup2D>(handle);
        return ptr != nullptr ? std::make_optional(std::reference_wrapper(*ptr)) : std::nullopt;
    }

    std::optional<std::reference_wrapper<RenderObject2D>> Scene2D::get_object(Handle handle) {
        auto *ptr = g_render_handle_table.deref<RenderObject2D>(handle);
        return ptr != nullptr ? std::make_optional(std::reference_wrapper(*ptr)) : std::nullopt;
    }

    Handle Scene2D::create_child_group(const Transform2D &transform) {
        return pimpl->root_group_write->create_child_group(transform);
    }

    Handle Scene2D::create_child_object(const std::string &material, const std::vector<RenderPrim2D> &primitives,
            const Vector2f &anchor_point, const Vector2f &atlas_stride, uint32_t z_index, float light_opacity,
            const Transform2D &transform) {
        return pimpl->root_group_write->create_child_object(material, primitives, anchor_point,
                atlas_stride, z_index, light_opacity, transform);
    }

    void Scene2D::remove_member_group(Handle handle) {
        pimpl->root_group_write->remove_member_group(handle);
    }

    void Scene2D::remove_member_object(Handle handle) {
        pimpl->root_group_write->remove_child_object(handle);
    }

    std::optional<std::reference_wrapper<Camera2D>> Scene2D::find_camera(const std::string &id) const {
        auto it = pimpl->cameras.find(id);
        return it != pimpl->cameras.cend() ? std::make_optional(std::reference_wrapper(it->second)) : std::nullopt;
    }

    Camera2D &Scene2D::create_camera(const std::string &id) {
        if (pimpl->cameras.find(id) != pimpl->cameras.end()) {
            throw std::invalid_argument("Camera with provided ID already exists in scene");
        }

        auto it = pimpl->cameras.insert({id, Camera2D(id, *this)});

        return it.first->second;
    }

    void Scene2D::destroy_camera(const std::string &id) {
        auto it = pimpl->cameras.find(id);
        if (it == pimpl->cameras.end()) {
            throw std::invalid_argument("Camera with provided ID does not exist in scene");
        }

        pimpl->cameras.erase(it);
    }
}
