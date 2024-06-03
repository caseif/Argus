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

#include "argus/lowlevel/atomic.hpp"
#include "argus/lowlevel/handle.hpp"
#include "argus/lowlevel/uuid.hpp"

#include "argus/render/common/transform.hpp"

#include <optional>
#include <string>
#include <vector>

namespace argus {
    // forward declarations
    class Material;

    class Scene;

    class RenderGroup2D;

    class RenderPrim2D;

    class Scene2D;

    struct pimpl_RenderObject2D;

    /**
     * \brief Represents an item to be rendered.
     * 
     * Each item specifies a material to be rendered with, which defines its
     * rendering properties.
     */
    class RenderObject2D {
      private:
        RenderObject2D(Handle handle, const RenderGroup2D &parent_group,
                const std::string &material, const std::vector<RenderPrim2D> &primitives,
                const Vector2f &anchor_point, const Vector2f &atlas_stride, uint32_t z_index,
                float light_opacity, const Transform2D &transform);

      public:
        pimpl_RenderObject2D *m_pimpl;

        RenderObject2D(const RenderGroup2D &parent_group, const std::string &material,
                const std::vector<RenderPrim2D> &primitives, const Vector2f &anchor_point,
                const Vector2f &atlas_stride, uint32_t z_index, float light_opacity,
                const Transform2D &transform);

        RenderObject2D(const RenderObject2D &) = delete;

        RenderObject2D(RenderObject2D &&) noexcept;

        ~RenderObject2D();

        /**
         * \brief Returns a persistent handle to the object.
         *
         * \return The object's handle.
         */
        [[nodiscard]] Handle get_handle(void) const;

        /**
         * \brief Gets the parent Scene of this object.
         *
         * \return The parent Scene.
         */
        [[nodiscard]] const Scene2D &get_scene(void) const;

        /**
         * \brief Gets the parent group of this object.
         *
         * \return The parent group.
         */
        [[nodiscard]] const RenderGroup2D &get_parent(void) const;

        /**
         * \brief Gets the UID of the Material used by the object.
         *
         * \return The UID of the Material used by the object.
         */
        [[nodiscard]] const std::string &get_material(void) const;

        /**
         * \brief Gets the \link RenderPrim RenderPrims \endlink comprising this
         *        object.
         *
         * \return The \link RenderPrim RenderPrims \endlink comprising this
         *         object.
         */
        [[nodiscard]] const std::vector<RenderPrim2D> &get_primitives(void) const;

        /**
         * \brief Gets the anchor point of the object about which rotation and
         *        scaling will be applied.
         * \return The anchor point of the object.
         */
        [[nodiscard]] const Vector2f &get_anchor_point(void) const;

        /**
         * \brief Gets the stride on each axis between atlas tiles, if the
         *        object has an animated texture.
         *
         * \return The stride between atlas tiles.
         */
        [[nodiscard]] const Vector2f &get_atlas_stride(void) const;

        /**
         * \brief Gets the z-index of the object. Objects with larger z-indices
         *        will be rendered in front of lower-indexed ones.
         *
         * \return THe z-index of the object.
         */
        [[nodiscard]] uint32_t get_z_index(void) const;

        /**
         * \brief Gets the opacity of the object with respect to lighting.
         *
         * 0.0 indicates an object which light will fully pass through while 1.0
         * indicates an object which no light will pass through.
         *
         * In practice this may be treated as a binary setting where values over
         * a certain threshold are treated as opaque and values under are
         * treated as translucent.
         *
         * \return The opacity of the object.
         */
        [[nodiscard]] float get_light_opacity(void) const;

        /**
         * \brief Sets the opacity of the object with respect to lighting.
         *
         * 0.0 indicates an object which light will fully pass through while 1.0
         * indicates an object which no light will pass through.
         *
         * In practice this may be treated as a binary setting where values over
         * a certain threshold are treated as opaque and values under are
         * treated as translucent.
         *
         * \param opacity The opacity of the object.
         */
        void set_light_opacity(float opacity);

        /**
         * \brief Gets the active animation frame.
         *
         * \return The x- and y-index of the currently active animation
         *        frame.
         */
        [[nodiscard]] ValueAndDirtyFlag<Vector2u> get_active_frame() const;

        /**
         * \brief Sets the active animation frame.
         *
         * \param frame The x- and y-index of the animation frame to
         *        activate. Neither index should exceed the number of tiles
         *        in each dimension in the atlas texture.
         */
        void set_active_frame(const Vector2u &frame);

        [[nodiscard]] const Transform2D &peek_transform(void) const;

        /**
         * \brief Gets the local Transform of this object.
         *
         * \return The local transform of this object.
         *
         * \remark The returned Transform is local and does not necessarily
         *         reflect the object's absolute transform with respect to
         *         the Scene containing the object.
         */
        Transform2D &get_transform(void);

        /**
         * \brief Sets the local Transform of this object.
         *
         * \param transform The new local Transform of the object.
         */
        void set_transform(const Transform2D &transform);

        RenderObject2D &copy(RenderGroup2D &parent);
    };
}
