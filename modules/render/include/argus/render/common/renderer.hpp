/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module core
#include "argus/lowlevel/time.hpp"

// module render
#include "argus/render/common/render_layer_type.hpp"

namespace argus {
    // forward declarations
    class RenderLayer;
    class TextureData;
    class Window;

    struct pimpl_Renderer;

    /**
     * \brief A construct which exposes functionality for rendering the entire
     *        screen space at once.
     *
     * Each Renderer has a one-to-one mapping with a Window, and a one-to-many
     * mapping with one or more \link RenderLayer RenderLayers \endlink.
     *
     * A Renderer is guaranteed to have at least one RenderLayer, considered to
     * be the "base" layer.
     *
     * \sa Window
     */
    class Renderer {
        public:
            pimpl_Renderer *const pimpl;

            /**
             * \brief Returns the Renderer associated with a given Window.
             *
             * \param window The Window to retrieve the Renderer for.
             *
             * \return The associated Renderer.
             */
            static Renderer &of_window(Window &window);

            /**
             * \brief Constructs a new Renderer attached to the given Window.
             *
             * \param window The Window to attach the new Renderer to.
             */
            Renderer(Window &window);

            Renderer(Renderer &rhs) = delete;

            Renderer(Renderer &&rhs) = delete;

            ~Renderer(void);

            Window &get_window() const;

            /**
             * \brief Initializes the Renderer.
             *
             * Initialization must be performed before render(TimeDelta) may be called.
             */
            void init(void);

            /**
             * \brief Outputs the Renderer's current state to the screen.
             *
             * \param delta The time in microseconds since the last frame.
             *
             * \remark This method accepts a TimeDelta to comply with the spec
             *         for engine callbacks as defined in the core module.
             */
            void render(const TimeDelta delta);

            /**
             * \brief Creates a new RenderLayer with the given priority.
             *
             * Layers with higher priority will be rendered after (ergo in front
             * of) those with lower priority.
             *
             * \param index The index of the new RenderLayer. Higher-indexed
             *        layers are rendered atop lower-indexed ones.
             *
             * \return The created RenderLayer.
             */
            RenderLayer &create_layer(const RenderLayerType type, const int index);

            /**
             * \brief Removes a RenderLayer from this Renderer, destroying it in
             *        the process.
             *
             * \param layer The child RenderLayer to remove.
             *
             * \throw std::invalid_argument If the supplied RenderLayer is not a
             *        child of this Renderer.
             */
            void remove_render_layer(RenderLayer &layer);
    };
}
