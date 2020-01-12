/*
 * This file is a part of Argus.
 * Copyright (c) 2019, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module renderer
#include "argus/renderer/render_group.hpp"
#include "argus/renderer/renderable_square.hpp"
#include "argus/renderer/renderable.hpp"
#include "argus/renderer/transform.hpp"

// module lowlevel
#include "argus/memory.hpp"

namespace argus {
    // forward declarations
    class RenderGroup;
    class Renderable;

    /**
     * \brief Represents a simple square to be rendered.
     *
     * \remark Squares are actually rendered to the screen as two adjacent
     *         triangles.
     */
    class RenderableSquare : public Renderable {
        friend class RenderableFactory;
        friend class AllocPool;

        private:
            const Vertex corner_1;
            const Vertex corner_2;
            const Vertex corner_3;
            const Vertex corner_4;

            /**
             * \brief Constructs a new RenderableSquare.
             *
             * \param RenderGroup The parent RenderGroup of the new
             *        RenderableSquare.
             * \param corner_1 The first corner of the new square.
             * \param corner_2 The second corner of the new square.
             * \param corner_3 The third corner of the new square.
             * \param corner_4 The fourth corner of the new square.
             */
            RenderableSquare(RenderGroup &parent_group, const Vertex &corner_1, const Vertex &corner_2,
                    const Vertex &corner_3, const Vertex &corner_4);

            void populate_buffer(void) override;

            const unsigned int get_vertex_count(void) const override;

            void free(void) override;
    };
}
