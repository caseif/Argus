/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module core
#include "argus/core.hpp"

// module render
#include "argus/render/render_layer.hpp"
#include "argus/render/window.hpp"

namespace argus {
    // forward declarations
    class RenderLayer;
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
        friend class Window;
        friend class pimpl_Window;

        private:
            pimpl_Renderer *pimpl;

            /**
             * \brief Constructs a new Renderer attached to the given Window.
             *
             * \param window The Window to attach the new Renderer to.
             */
            Renderer(Window &window);

            Renderer(Renderer &rhs) = delete;

            Renderer(Renderer &&rhs) = delete;

            ~Renderer(void);

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

        public:
            /**
             * \brief Destroys this renderer.
             *
             * \warning This method destroys the Renderer object. No other
             *          methods should be invoked upon it afterward.
             */
            void destroy(void);

            /**
             * \brief Creates a new RenderLayer with the given priority.
             *
             * Layers with higher priority will be rendered after (ergo in front
             * of) those with lower priority.
             *
             * \param priority The priority of the new RenderLayer.
             *
             * \return The created RenderLayer.
             */
            RenderLayer &create_render_layer(const int priority);

            /**
             * \brief Removes a render layer from this renderer and destroys it.
             *
             * \param layer The child RenderLayer to remove.
             */
            void remove_render_layer(RenderLayer &layer);
    };
}
