/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module renderer
#include "argus/renderer/render_group.hpp"
#include "argus/renderer/renderable_square.hpp"
#include "argus/renderer/renderable_triangle.hpp"
#include "argus/renderer/transform.hpp"

namespace argus {
    // forward declarations
    class RenderGroup;
    class RenderableSquare;
    class RenderableTriangle;

    /**
     * \brief Provides methods for creating new \link Renderable Renderables
     *        \endlink associated with a particular RenderGroup.
     */
    class RenderableFactory {
        friend class RenderGroup;
        friend class pimpl_RenderGroup;

        private:
            RenderGroup &parent;

            RenderableFactory(RenderGroup &parent);

        public:
            /**
             * \brief Creates a new RenderableTriangle with the given vertices.
             *
             * \param corner_1 The first corner of the triangle.
             * \param corner_2 The second corner of the triangle.
             * \param corner_3 The third corner of the triangle.
             *
             * \return The created RenderableTriangle.
             */
            RenderableTriangle &create_triangle(const Vertex &corner_1, const Vertex &corner_2, const Vertex &corner_3) const;

            /**
             * \brief Creates a new RenderableSquare with the given vertices.
             *
             * \param corner_1 The first corner of the square.
             * \param corner_2 The second corner of the square.
             * \param corner_3 The third corner of the square.
             * \param corner_4 The fourth corner of the square.
             *
             * \return The created RenderableTriangle.
             */
            RenderableSquare &create_square(const Vertex &corner_1, const Vertex &corner_2,
                    const Vertex &corner_3, const Vertex &corner_4) const;
    };
}
