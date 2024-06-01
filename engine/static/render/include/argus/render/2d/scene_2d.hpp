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

#pragma once

#include "argus/lowlevel/handle.hpp"

#include "argus/render/common/scene.hpp"
#include "argus/render/common/transform.hpp"
#include "argus/render/2d/light_2d.hpp"

#include <optional>
#include <string>
#include <vector>

namespace argus {
    // forward declarations
    class Material;
    class Canvas;
    class Camera2D;
    class Light2D;
    class RenderGroup2D;
    class RenderObject2D;
    class RenderPrim2D;

    struct pimpl_Scene2D;

    /**
     * \brief Represents a scene which contains a set of geometry in
     *        2-dimensional space.
     *
     * Scenes are composited to the screen as stacked layers when a frame is
     * rendered.
     */
    class Scene2D : public Scene {
      private:
        /**
         * \brief Constructs a new Scene2D.
         *
         * \param id The ID of the Scene.
         * \param transform The Transform of the Scene.
         */
        Scene2D(const std::string &id, const Transform2D &transform);

        Scene2D(const std::string &id, Transform2D &&transform) :
                Scene2D(id, transform) {
        }

        Scene2D(Scene2D &&) noexcept;

        ~Scene2D(void) override;

      public:
        static Scene2D &create(const std::string &id);

        pimpl_Scene2D *m_pimpl;

        Scene2D(const Scene2D &) noexcept = delete;

        [[nodiscard]] pimpl_Scene *get_pimpl(void) const override;

        [[nodiscard]] bool is_lighting_enabled(void);

        void set_lighting_enabled(bool enabled);

        [[nodiscard]] float peek_ambient_light_level(void) const;

        [[nodiscard]] ValueAndDirtyFlag<float> get_ambient_light_level(void);

        void set_ambient_light_level(float level);

        [[nodiscard]] const Vector3f &peek_ambient_light_color(void) const;

        [[nodiscard]] ValueAndDirtyFlag<Vector3f> get_ambient_light_color(void);

        void set_ambient_light_color(const Vector3f &color);

        std::vector<std::reference_wrapper<Light2D>> get_lights(void);

        std::vector<std::reference_wrapper<const Light2D>> get_lights_for_render(void);

        Handle add_light(Light2DType type, bool is_occludable, const Vector3f &color,
                LightParameters params, const Transform2D &iniital_transform);

        std::optional<std::reference_wrapper<Light2D>> get_light(Handle handle);

        void remove_light(Handle handle);

        std::optional<std::reference_wrapper<RenderGroup2D>> get_group(Handle handle);

        std::optional<std::reference_wrapper<RenderObject2D>> get_object(Handle handle);

        /**
         * \brief Creates a new RenderGroup2D as a direct child of this
         *        Scene.
         *
         * \param transform The relative transform of the new group.
         */
        Handle add_group(const Transform2D &transform);

        /**
         * \brief Creates a new RenderObject2D as a direct child of this
         *        Scene.
         *
         * \param material The Material to be used by the new object.
         * \param primitives The primitives comprising the new object.
         * \param transform The relative transform of the new object.
         *
         * \remark Internally, the object will be created as a child of the
         *         implicit root RenderGroup contained by this Scene. Thus,
         *         no RenderObject is truly without a parent group.
         */
        Handle add_object(const std::string &material, const std::vector<RenderPrim2D> &primitives,
                const Vector2f &anchor_point, const Vector2f &atlas_stride, uint32_t z_index, float light_opacity,
                const Transform2D &transform);

        /**
         * \brief Removes the supplied RenderGroup2D from this Scene,
         *        destroying it in the process.
         * \param handle The handle to the group to remove and destroy.
         * \throw std::invalid_argument If the supplied RenderGroup is not a
         *        direct member of this Scene.
         */
        void remove_group(Handle handle);

        /**
         * \brief Removes the specified RenderObject2D from this Scene,
         *        destroying it in the process.
         * \param handle The handle to the RenderObject2D to remove and destroy.
         * \throw std::invalid_argument If the supplied RenderObject is not
         *        a direct member of this Scene.
         */
        void remove_object(Handle handle);

        [[nodiscard]] std::optional<std::reference_wrapper<Camera2D>> find_camera(const std::string &id) const;

        Camera2D &create_camera(const std::string &id);

        void destroy_camera(const std::string &id);

        void lock_render_state(void);

        void unlock_render_state(void);
    };
}
