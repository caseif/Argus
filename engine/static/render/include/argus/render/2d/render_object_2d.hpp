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
                const Transform2D &transform);

      public:
        pimpl_RenderObject2D *pimpl;

        RenderObject2D(const RenderGroup2D &parent_group, const std::string &material,
                const std::vector<RenderPrim2D> &primitives, const Vector2f &anchor_point,
                const Vector2f &atlas_stride, uint32_t z_index, const Transform2D &transform);

        RenderObject2D(const RenderObject2D &) = delete;

        RenderObject2D(RenderObject2D &&) noexcept;

        ~RenderObject2D();

        /**
         * \brief Gets the parent Scene of this object.
         *
         * \return The parent Scene.
         */
        const Scene2D &get_scene(void) const;

        /**
         * \brief Gets the UID of the Material used by the object.
         *
         * \return The UID of the Material used by the object.
         */
        const std::string &get_material(void) const;

        /**
         * \brief Gets the \link RenderPrim RenderPrims \endlink comprising this
         *        object.
         *
         * \return The \link RenderPrim RenderPrims \endlink comprising this
         *         object.
         */
        const std::vector<RenderPrim2D> &get_primitives(void) const;

        /**
         * \brief Gets the anchor point of the object about which rotation and
         *        scaling will be applied.
         * \return The anchor point of the object.
         */
        const Vector2f &get_anchor_point(void) const;

        /**
         * \brief Gets the stride on each axis between atlas tiles, if the
         *        object has an animated texture.
         *
         * \return The stride between atlas tiles.
         */
        const Vector2f &get_atlas_stride(void) const;

        /**
         * \brief Gets the z-index of the object. Objects with larger z-indices
         *        will be rendered in front of lower-indexed ones.
         *
         * \return THe z-index of the object.
         */
        uint32_t get_z_index(void) const;

        /**
         * \brief Gets the active animation frame.
         *
         * \return The x- and y-index of the currently active animation
         *        frame.
         */
        Dirtiable<Vector2u> get_active_frame() const;

        /**
         * \brief Sets the active animation frame.
         *
         * \param frame The x- and y-index of the animation frame to
         *        activate. Neither index should exceed the number of tiles
         *        in each dimension in the atlas texture.
         */
        void set_active_frame(const Vector2u &frame);

        const Transform2D &peek_transform(void) const;

        /**
         * \brief Gets the local Transform of this object.
         *
         * \return The local transform of this object.
         *
         * \remark The returned Transform is local and does not necessarily
         *         reflect the object's absolute transform with respect to
         *         the Scene containing the object.
         */
        ValueAndDirtyFlag<Transform2D> get_transform(void);

        /**
         * \brief Sets the local Transform of this object.
         *
         * \param transform The new local Transform of the object.
         */
        void set_transform(const Transform2D &transform) const;

        RenderObject2D &copy(RenderGroup2D &parent);
    };
}
