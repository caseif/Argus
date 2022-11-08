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

#pragma once

#include "argus/lowlevel/dirtiable.hpp"

#include "argus/render/common/transform.hpp"

#include <map>
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
            RenderGroup2D &copy(RenderGroup2D *parent);

        public:
            pimpl_RenderGroup2D *pimpl;

            /**
             * \brief Constructs a new RenderGroup.
             *
             * \param scene The Scene this group belongs to.
             * \param parent_group The parent group this group belongs to, if
             *        applicable. This may be nullptr.
             */
            RenderGroup2D(Scene2D &scene, RenderGroup2D *parent_group, const Transform2D &transform);

            RenderGroup2D(Scene2D &scene, RenderGroup2D *parent_group, Transform2D &&transform);

            RenderGroup2D(Scene2D &scene, RenderGroup2D *parent_group);

            RenderGroup2D(const RenderGroup2D&) = delete;

            RenderGroup2D(RenderGroup2D&&) noexcept;

            ~RenderGroup2D(void);

            /**
             * \brief Gets the parent Scene.
             *
             * \return The parent Scene to this group.
             */
            Scene2D &get_scene(void) const;

            /**
             * \brief Gets the parent RenderGroup, if applicable.
             *
             * \return The parent group to this one, or nullptr if this is a
             *         root group.
             */
            RenderGroup2D *get_parent_group(void) const;

            /**
             * \brief Creates a new RenderGroup as a child of this group.
             *
             * \param transform The relative transform of the new group.
             */
            RenderGroup2D &create_child_group(const Transform2D &transform);

            /**
             * \brief Creates a new RenderObject as a child of this group.
             *
             * \param material The Material to be used by the new object.
             * \param primitives The \link RenderPrim primitives \endlink
             *        comprising the new object.
             * \param transform The relative transform of the new object.
             */
            RenderObject2D &create_child_object(const std::string &material,
                    const std::vector<RenderPrim2D> &primitives, const Transform2D &transform);

            /**
             * \brief Removes the supplied RenderGroup from this group,
             *        destroying it in the process.
             *
             * \param group The group to remove and destroy.
             *
             * \throw std::invalid_argument If the supplied RenderGroup is not a
             *        child of this group.
             */
            void remove_member_group(RenderGroup2D &group);

            /**
             * \brief Removes the specified RenderObject from this group,
             *        destroying it in the process.
             *
             * \param object The RenderObject to remove and destroy.
             *
             * \throw std::invalid_argument If the supplied RenderObject is not
             *        a child of this group.
             */
            void remove_child_object(RenderObject2D &object);

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
            const Transform2D &peek_transform(void) const;

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
            ValueAndDirtyFlag<Transform2D> get_transform(void);

            /**
             * Sets the local Transform of this group.
             *
             * \param transform The new local Transform for this group.
             */
            void set_transform(const Transform2D &transform);

            RenderGroup2D &copy(void);
    };
}
