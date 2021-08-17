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
#include "argus/render/common/render_layer_type.hpp"

namespace argus {
    class Renderer;
    struct pimpl_RenderLayer;
class Transform2D;

    /**
     * \brief Represents a layer to which geometry may be rendered.
     *
     * RenderLayers will be composited to the screen as multiple ordered layers
     * when a frame is rendered.
     */
    class RenderLayer {
        protected:
            /**
             * \brief Constructs a new RenderLayer.
             *
             * \param type The type of layer.
             */
            RenderLayer(const RenderLayerType type);

            RenderLayer(const RenderLayer &rhs) = delete;

            RenderLayer(const RenderLayer &&rhs) = delete;

        public:
            virtual ~RenderLayer(void) = 0;

            const RenderLayerType type;

            virtual pimpl_RenderLayer *get_pimpl(void) const = 0;

            /**
             * \brief Gets the parent Renderer of this layer.
             */
            const Renderer &get_parent_renderer(void) const;

            /**
             * \brief Gets the Transform of this layer.
             *
             * \return The layer's Transform.
             */
            Transform2D &get_transform(void) const;

            /**
             * \brief Sets the Transform of this layer.
             *
             * \param transform The new Transform.
             */
            void set_transform(Transform2D &transform);
    };
}
