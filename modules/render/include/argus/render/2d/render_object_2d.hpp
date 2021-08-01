/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module render
#include "argus/render/common/transform.hpp"

#include <vector>

namespace argus {
    // forward declarations
    class Material;
    class RenderLayer;

    class RenderGroup2D;
    class RenderPrim2D;

    struct pimpl_RenderObject2D;

    /**
     * \brief Represents an item to be rendered.
     * 
     * Each item specifies a material to be rendered with, which defines its
     * rendering properties.
     */
    class RenderObject2D {
        public:
            pimpl_RenderObject2D *pimpl;

            RenderObject2D(const RenderGroup2D &parent_group, const Material &material,
                    const std::vector<RenderPrim2D> &primitives, const Transform2D &transform);

            RenderObject2D(const RenderObject2D&) noexcept;

            RenderObject2D(RenderObject2D&&) noexcept;

            ~RenderObject2D();

            /**
             * \brief Gets the parent RenderLayer of this object.
             *
             * \return The parent RenderLayer.
             */
            const RenderLayer2D &get_parent_layer(void) const;

            /**
             * \brief Gets the Material used by this object.
             *
             * \return The Material used by this object.
             */
            const Material &get_material(void) const;

            /**
             * \brief Gets the \link RenderPrim RenderPrims \endlink comprising this
             *        object.
             *
             * \return The \link RenderPrim RenderPrims \endlink comprising this
             *         object.
             */
            const std::vector<RenderPrim2D> &get_primitives(void) const;

            /**
             * \brief Gets the local Transform of this object.
             *
             * \return The local transform of this object.
             *
             * \remark The returned Transform is local and does not necessarily
             *         reflect the object's absolute transform with respect to
             *         the RenderLayer containing the object.
             */
            Transform2D &get_transform(void) const;
            
            /**
             * \brief Sets the local Transform of this object.
             *
             * \param transform The new local Transform of the object.
             */
            void set_transform(Transform2D &transform) const;
    };
}
