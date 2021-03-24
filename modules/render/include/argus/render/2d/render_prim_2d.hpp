/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module lowlevel
#include "argus/lowlevel/math.hpp"

// module render
#include "argus/render/common/transform.hpp"
#include "argus/render/common/vertex.hpp"

#include <atomic>
#include <string>
#include <vector>

#include <cstddef>
#include <cstdio>

namespace argus {
    // forward declarations

    struct pimpl_RenderPrim2D;

    /**
     * \brief Represents primitive 2D geometry to be rendered.
     */
    class RenderPrim2D {
        public:
            pimpl_RenderPrim2D *pimpl;

            /**
             * \brief Constructs a new RenderPrim2D object.
             *
             * \param vertices The vertices comprising the new primitive.
             */
            RenderPrim2D(const std::vector<Vertex2D> &vertices);

            /**
             * \brief Constructs a new RenderPrim2D object.
             *
             * \param vertices The vertices comprising the new primitive.
             */
            RenderPrim2D(const std::initializer_list<Vertex2D> vertices);

            RenderPrim2D(void) = delete;

            RenderPrim2D(const RenderPrim2D&) noexcept;

            RenderPrim2D(RenderPrim2D&&) noexcept;

            ~RenderPrim2D(void);

            /**
             * \brief Gets the current vertex count of this RenderPrim2D.
             *
             * \return The current vertex count of this RenderPrim2D.
             */
            const size_t get_vertex_count(void) const;
    };
}
