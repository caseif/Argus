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

#pragma once

#include "argus/render/common/scene.hpp"
#include "argus/render/common/transform.hpp"

#include <string>
#include <vector>

namespace argus {
    // forward declarations
    class Material;
    class Canvas;

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
        public:
            pimpl_Scene2D *pimpl;

            /**
             * \brief Constructs a new Scene2D.
             *
             * \param parent The Canvas parent to the Scene.
             * \param transform The Transform of the Scene.
             * \param index The compositing index of the Scene. Higher-indexed
             *        Scenes are rendered on top of lower-indexed ones.
             */
            Scene2D(const Canvas &canvas, const Transform2D &transform, int index);

            Scene2D(const Canvas &canvas, Transform2D &&transform, int index):
                Scene2D(canvas, transform, index) {
            }

            Scene2D(const Scene2D&) noexcept;

            Scene2D(Scene2D&&) noexcept;

            ~Scene2D(void);

            pimpl_Scene *get_pimpl(void) const override;

            /**
             * \brief Creates a new RenderGroup2D as a direct child of this
             *        Scene.
             *
             * \param transform The relative transform of the new group.
             */
            RenderGroup2D &create_child_group(const Transform2D &transform);

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
            RenderObject2D &create_child_object(const std::string &material,
                    const std::vector<RenderPrim2D> &primitives, const Transform2D &transform);

            /**
             * \brief Removes the supplied RenderGroup2D from this Scene,
             *        destroying it in the process.
             * \param group The group to remove and destroy.
             * \throw std::invalid_argument If the supplied RenderGroup is not a
             *        direct member of this Scene.
             */
            void remove_member_group(RenderGroup2D &group);

            /**
             * \brief Removes the specified RenderObject2D from this Scene,
             *        destroying it in the process.
             * \param object The RenderObject2D to remove and destroy.
             * \throw std::invalid_argument If the supplied RenderObject is not
             *        a direct member of this Scene.
             */
            void remove_member_object(RenderObject2D &object);
    };
}
