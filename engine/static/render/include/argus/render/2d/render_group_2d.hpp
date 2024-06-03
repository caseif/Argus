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

#include "argus/lowlevel/dirtiable.hpp"
#include "argus/lowlevel/handle.hpp"

#include "argus/render/common/transform.hpp"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace argus {
    // forward declarations
    class Material;

    class Canvas;

    class Scene;

    class RenderGroup2D;

    class RenderObject2D;

    class RenderPrim2D;

    class Scene2D;

    struct pimpl_RenderGroup2D;

    /**
     * \brief Represents a set of RenderGroups and RenderObjects to be rendered
     *        together.
     *
     * A RenderGroup supplies a Transform which will be applied when rendering
     * child groups/objects in addition to their own local Transform.
     */
    class RenderGroup2D {
      private:
        RenderGroup2D(Handle handle, Scene2D &scene, RenderGroup2D *parent_group,
                const Transform2D &transform);

        RenderGroup2D &copy(RenderGroup2D *parent);

      public:
        pimpl_RenderGroup2D *m_pimpl;

        /**
         * \brief Constructs a new RenderGroup.
         *
         * \param scene The Scene this group belongs to.
         * \param parent_group The parent group this group belongs to, if
         *        applicable. This may be nullptr.
         */
        RenderGroup2D(Scene2D &scene, RenderGroup2D *parent_group,
                const Transform2D &transform);

        RenderGroup2D(Scene2D &scene, RenderGroup2D *parent_group,
                Transform2D &&transform);

        RenderGroup2D(Scene2D &scene, RenderGroup2D *parent_group);

        RenderGroup2D(const RenderGroup2D &) = delete;

        RenderGroup2D(RenderGroup2D &&) noexcept;

        ~RenderGroup2D(void);

        /**
         * \brief Returns a persistent handle to the object.
         *
         * \return The object's handle.
         */
        [[nodiscard]] Handle get_handle(void) const;

        /**
         * \brief Gets the parent Scene.
         *
         * \return The parent Scene to this group.
         */
        [[nodiscard]] Scene2D &get_scene(void) const;

        /**
         * \brief Gets the parent RenderGroup, if applicable.
         *
         * \return The parent group to this one, or nullptr if this is a
         *         root group.
         */
        [[nodiscard]] std::optional<std::reference_wrapper<RenderGroup2D>> get_parent(void) const;

        /**
         * \brief Creates a new RenderGroup as a child of this group.
         *
         * \param transform The relative transform of the new group.
         */
        Handle add_group(const Transform2D &transform);

        /**
         * \brief Creates a new RenderObject as a child of this group.
         *
         * \param material The Material to be used by the new object.
         * \param primitives The \link RenderPrim primitives \endlink
         *        comprising the new object.
         * \param transform The relative transform of the new object.
         */
        Handle add_object(const std::string &material,
                const std::vector<RenderPrim2D> &primitives,
                const Vector2f &anchor_point, const Vector2f &atlas_stride,
                uint32_t z_index, float light_opacity, const Transform2D &transform);

        /**
         * \brief Removes the specified child group from this group,
         *        destroying it in the process.
         *
         * \param handle The handle of the group to remove and destroy.
         *
         * \throw std::invalid_argument If the given ID does not match a
         *        group in the scene or if the matching group is not a child
         *        of this group.
         */
        void remove_group(Handle handle);

        /**
         * \brief Removes the specified object from this group,
         *        destroying it in the process.
         *
         * \param handle The handle of the object remove and destroy.
         *
         * \throw std::invalid_argument If the given ID does not match an
         *        object in the scene or if the matching object is not a
         *        child of this group.
         */
        void remove_object(Handle handle);

        /**
         * \brief Peeks the local Transform of this group without clearing
         *        its dirty flag.
         *
         * \return The local Transform.
         *
         * \remark The returned Transform is local and, if this group is a
         *         child of another, does not necessarily reflect the
         *         group's absolute transform with respect to the
         *         Scene containing the group.
         */
        [[nodiscard]] const Transform2D &peek_transform(void) const;

        /**
         * \brief Gets the local Transform of this group and clears its
         *        dirty flag.
         *
         * \return The local Transform and its dirty flag.
         *
         * \remark The returned Transform is local and, if this group is a
         *         child of another, does not necessarily reflect the
         *         group's absolute transform with respect to the
         *         Scene containing the group.
         */
        Transform2D &get_transform(void);

        /**
         * Sets the local Transform of this group.
         *
         * \param transform The new local Transform for this group.
         */
        void set_transform(const Transform2D &transform);

        RenderGroup2D &copy(void);
    };
}
