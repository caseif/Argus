/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module lowlevel
#include "argus/lowlevel/math.hpp"

// module render
#include "argus/render/transform.hpp"

#include <atomic>
#include <string>
#include <vector>

#include <cstdio>

namespace argus {
    // forward declarations
    class Renderable;

    struct pimpl_RenderPrim;

    /**
     * \brief Represents primitive geometry to be rendered.
     */
    class RenderPrim {
        public:
            pimpl_RenderPrim *const pimpl;

            /**
             * \brief Constructs a new RenderPrim object.
             *
             * \param vertices The vertices comprising the new primitive.
             */
            RenderPrim(const std::vector<Vertex> &vertices);

            /**
             * \brief Constructs a new RenderPrim object.
             *
             * \param vertices The vertices comprising the new primitive.
             */
            RenderPrim(const std::initializer_list<Vertex> vertices);

            ~RenderPrim(void);

            /**
             * \brief Gets the current vertex count of this RenderPrim.
             *
             * \return The current vertex count of this RenderPrim.
             */
            const size_t get_vertex_count(void) const;
    };
}
