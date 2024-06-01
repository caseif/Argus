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

#include "argus/lowlevel/color.hpp"
#include "argus/lowlevel/memory.hpp"

#include "argus/render/2d/camera_2d.hpp"
#include "argus/render/common/scene.hpp"
#include "argus/render/2d/render_group_2d.hpp"
#include "argus/render/2d/render_object_2d.hpp"
#include "argus/render/2d/scene_2d.hpp"
#include "internal/render/common/scene.hpp"
#include "internal/render/pimpl/2d/light_2d.hpp"
#include "internal/render/pimpl/2d/render_object_2d.hpp"
#include "internal/render/pimpl/2d/scene_2d.hpp"
#include "internal/render/module_render.hpp"

#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>

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
            m_pimpl(&g_pimpl_pool.construct<pimpl_Scene2D>(id, *this, transform)) {
    }

    Scene2D::Scene2D(Scene2D &&rhs) noexcept:
            Scene(SceneType::TwoD),
            m_pimpl(rhs.m_pimpl) {
        rhs.m_pimpl = nullptr;
    }

    Scene2D::~Scene2D(void) {
        g_scenes.erase(g_scenes.find(m_pimpl->id), g_scenes.end());

        if (m_pimpl != nullptr) {
            g_pimpl_pool.destroy(m_pimpl);
        }
    }

    pimpl_Scene *Scene2D::get_pimpl(void) const {
        return dynamic_cast<pimpl_Scene *>(m_pimpl);
    }

    bool Scene2D::is_lighting_enabled(void) {
        return m_pimpl->lighting_enabled;
    }

    void Scene2D::set_lighting_enabled(bool enabled) {
        m_pimpl->lighting_enabled = enabled;
    }

    float Scene2D::peek_ambient_light_level(void) const {
        return m_pimpl->ambient_light_level.peek();
    }

    ValueAndDirtyFlag<float> Scene2D::get_ambient_light_level(void) {
        return m_pimpl->ambient_light_level.read();
    }

    void Scene2D::set_ambient_light_level(float level) {
        m_pimpl->ambient_light_level = level;
    }

    const Vector3f &Scene2D::peek_ambient_light_color(void) const {
        return m_pimpl->ambient_light_color.peek();
    }

    ValueAndDirtyFlag<Vector3f> Scene2D::get_ambient_light_color(void) {
        return m_pimpl->ambient_light_color.read();
    }

    void Scene2D::set_ambient_light_color(const Vector3f &color) {
        m_pimpl->ambient_light_color = normalize_rgb(color);
    }

    std::vector<std::reference_wrapper<Light2D>> Scene2D::get_lights(void) {
        std::vector<std::reference_wrapper<Light2D>> res;
        res.reserve(m_pimpl->lights_staging->size());
        std::transform(m_pimpl->lights_staging->begin(), m_pimpl->lights_staging->end(), std::back_inserter(res),
                [](auto &kvp) { return std::reference_wrapper(kvp.second); });
        return res;
    }

    std::vector<std::reference_wrapper<const Light2D>> Scene2D::get_lights_for_render(void) {
        std::vector<std::reference_wrapper<const Light2D>> res;
        res.reserve(m_pimpl->lights->size());
        std::transform(m_pimpl->lights->cbegin(), m_pimpl->lights->cend(), std::back_inserter(res),
                [](const auto &kvp) { return std::reference_wrapper(kvp.second); });
        return res;
    }

    Handle Scene2D::add_light(Light2DType type, bool is_occludable, const Vector3f &color,
            LightParameters params, const Transform2D &iniital_transform) {
        Light2D light(type, is_occludable, normalize_rgb(color), params, iniital_transform);
        auto inserted = m_pimpl->lights_staging->insert({ light.get_handle(), light });
        return inserted.first->first;
    }

    std::optional<std::reference_wrapper<Light2D>> Scene2D::get_light(Handle handle) {
        auto *ptr = g_render_handle_table.deref<Light2D>(handle);
        return ptr != nullptr ? std::make_optional(std::reference_wrapper(*ptr)) : std::nullopt;
    }

    void Scene2D::remove_light(Handle handle) {
        m_pimpl->lights_staging->erase(handle);
    }

    std::optional<std::reference_wrapper<RenderGroup2D>> Scene2D::get_group(Handle handle) {
        auto *ptr = g_render_handle_table.deref<RenderGroup2D>(handle);
        return ptr != nullptr ? std::make_optional(std::reference_wrapper(*ptr)) : std::nullopt;
    }

    std::optional<std::reference_wrapper<RenderObject2D>> Scene2D::get_object(Handle handle) {
        auto *ptr = g_render_handle_table.deref<RenderObject2D>(handle);
        return ptr != nullptr ? std::make_optional(std::reference_wrapper(*ptr)) : std::nullopt;
    }

    Handle Scene2D::add_group(const Transform2D &transform) {
        return m_pimpl->root_group_write->add_group(transform);
    }

    Handle Scene2D::add_object(const std::string &material, const std::vector<RenderPrim2D> &primitives,
            const Vector2f &anchor_point, const Vector2f &atlas_stride, uint32_t z_index, float light_opacity,
            const Transform2D &transform) {
        return m_pimpl->root_group_write->add_object(material, primitives, anchor_point,
                atlas_stride, z_index, light_opacity, transform);
    }

    void Scene2D::remove_group(Handle handle) {
        m_pimpl->root_group_write->remove_group(handle);
    }

    void Scene2D::remove_object(Handle handle) {
        m_pimpl->root_group_write->remove_object(handle);
    }

    std::optional<std::reference_wrapper<Camera2D>> Scene2D::find_camera(const std::string &id) const {
        auto it = m_pimpl->cameras.find(id);
        return it != m_pimpl->cameras.cend() ? std::make_optional(std::reference_wrapper(it->second)) : std::nullopt;
    }

    Camera2D &Scene2D::create_camera(const std::string &id) {
        if (m_pimpl->cameras.find(id) != m_pimpl->cameras.end()) {
            throw std::invalid_argument("Camera with provided ID already exists in scene");
        }

        auto it = m_pimpl->cameras.insert({id, Camera2D(id, *this)});

        return it.first->second;
    }

    void Scene2D::destroy_camera(const std::string &id) {
        auto it = m_pimpl->cameras.find(id);
        if (it == m_pimpl->cameras.end()) {
            throw std::invalid_argument("Camera with provided ID does not exist in scene");
        }

        m_pimpl->cameras.erase(it);
    }

    void Scene2D::lock_render_state(void) {
        m_pimpl->read_lock.lock();
    }

    void Scene2D::unlock_render_state(void) {
        m_pimpl->read_lock.unlock();
    }
}
