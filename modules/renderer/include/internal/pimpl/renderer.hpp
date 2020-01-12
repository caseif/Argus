#include "argus/renderer.hpp"

namespace argus {
    struct pimpl_Renderer {
        /**
         * \brief The Window which this Renderer is mapped to.
         */
        Window &window;
        /**
         * \brief The child \link RenderLayer RenderLayers \endlink of this
         *        Renderer.
         */
        std::vector<RenderLayer*> render_layers;

        /**
         * \brief The graphics context associated with this Renderer.
         */
        graphics_context_t gfx_context;
        /**
         * \brief The ID of the engine callback registered for this
         *        Renderer.
         */
        Index callback_id;

        /**
         * \brief Whether this Renderer has been initialized.
         */
        bool initialized;
        /**
         * \brief Whether this Renderer is queued for destruction.
         */
        std::atomic_bool destruction_pending;
        /**
         * \brief Whether this Renderer is still valid.
         *
         * If `false`, the Renderer has been destroyed.
         */
        bool valid;

        /**
         * \brief Whether the render resolution has recently been updated.
         */
        std::atomic_bool dirty_resolution;

        pimpl_Renderer(Window &window):
                window(window) {
        }
    };
}