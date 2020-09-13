/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module render
#include "argus/render/render_group.hpp"
#include "argus/render/renderable.hpp"
#include "argus/render/transform.hpp"

// module lowlevel
#include "argus/lowlevel/memory.hpp"

namespace argus {
    // forward declarations
    class RenderGroup;
    class Renderable;

    /**
     * \brief Represents a simple triangle to be rendered.
     */
    class RenderableTriangle : public Renderable {
        friend class RenderableFactory;
        friend class AllocPool;

        private:
            const Vertex corner_1;
            const Vertex corner_2;
            const Vertex corner_3;

            /**
             * \brief Constructs a new RenderableTriangle.
             *
             * \param RenderGroup The parent RenderGroup of the new
             *        RenderableTriangle.
             * \param corner_1 The first corner of the new triangle.
             * \param corner_2 The second corner of the new triangle.
             * \param corner_3 The third corner of the new triangle.
             */
            RenderableTriangle(RenderGroup &parent_group, const Vertex &corner_1, const Vertex &corner_2,
                    const Vertex &corner_3);

            void populate_buffer(void) override;

            const unsigned int get_vertex_count(void) const override;

            void free(void) override;
    };
}
