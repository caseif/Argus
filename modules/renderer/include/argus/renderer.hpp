#pragma once

// module core
#include "argus/core.hpp"

#define VMMLIB_OLD_TYPEDEFS
#include "vmmlib/matrix.hpp"
#include "vmmlib/vector.hpp"

#include <functional>
#include <string>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_render.h>

namespace argus {

    // forward declarations for friend declarations
    class Renderer;
    class RenderLayer;

    class Window {
        friend class Renderer;

        private:
            SDL_Window *handle;

            uint64_t callback_id;
            uint64_t listener_id;

            Window *parent;
            std::vector<Window*> children;

            Renderer *renderer;

            Window(void);
            
            ~Window(void);

            void remove_child(Window &child);

            void update(const Timestamp delta);

            static int event_filter(void *data, SDL_Event *event);
            
            static void update_window(const Window &window, const Timestamp delta);

        public:
            /**
             * \brief Creates a new window.
             *
             * \return The new window.
             */
            static Window &create_window(void);

            /**
             * \brief Destroys this window.
             *
             * Warning: This method destroys the Window object. No other
             * methods should be invoked upon it after calling destroy().
             */
            void destroy(void);

            /**
             * \brief Creates a new window as a child of this one.
             *
             * Note that the child window will not be modal to the parent.
             *
             * \return The new child window.
             */
            Window &create_child_window(void);

            /**
             * Gets this Window's associated Renderer.
             *
             * \return The Window's Renderer.
             */
            Renderer &get_renderer(void) const;

            /**
             * Sets the window title.
             *
             * \param title The new window title.
             */
            void set_title(const std::string &title);

            /**
             * \brief Sets the fullscreen state of the window.
             *
             * Caution: This may not be supported on all platforms.
             *
             * \param fullscreen Whether the window is to be displayed in
             *                   fullscreen.
             */
            void set_fullscreen(const bool fullscreen);

            /**
             * \brief Sets the resolution of the window when not in fullscreen
             *        mode.
             *
             * Caution: This may not be supported on all platforms.
             *
             * \param width The new width of the window.
             * \param height The new height of the window.
             */
            void set_resolution(const unsigned int width, const unsigned int height);

            /**
             * \brief Sets the position of the window on the screen when in
             *        windowed mode.
             *
             * Caution: This may not be supported on all platforms.
             *
             * \param x The new X-coordinate of the window.
             * \param y The new Y-coordinate of the window.
             */
            void set_windowed_position(const unsigned int x, const unsigned int y);

            /*
             * \brief Activates the window.
             *
             * This function should be invoked only once.
             */
            void activate(void);
    };

    class Transform {
        private:
            Transform *parent;
            vmml::vec2d translation;
            double rotation;
            vmml::vec2d scale;

        public:
            Transform(void);

            Transform(const vmml::vec2d translation, const double rotation, const vmml::vec2d scale);

            ~Transform(void);

            const vmml::vec2d &get_translation(void) const;

            void set_translation(vmml::vec2d &translation);

            void add_translation(vmml::vec2d &translation_delta);
            
            const double get_rotation(void) const;

            void set_rotation(const double rotation_degrees);

            void add_rotation(const double rotation_degrees);

            const vmml::vec2d &get_scale(void) const;

            void set_scale(vmml::vec2d &scale);

            void set_parent(Transform &transform);

            const vmml::mat3d &to_matrix(void) const;
    };

    /**
     * \brief Represents an item to be rendered.
     *
     * Each item may have its own rendering properties, as well as a list of
     * child items. Child items will inherit the transform of their parent,
     * which is added to their own.
     */
    class RenderItem {
        friend class RenderLayer;

        private:
            ~RenderItem(void) = default;

        protected:
            RenderLayer &layer;
            RenderItem *const parent; // can be null
            std::vector<RenderItem*> children;

            RenderItem(RenderLayer &layer, RenderItem *const parent);

            void render_children(void) const;

        public:
            void render(void) const;

            void destroy(void);
    };

    /**
     * Represents a layer to which geometry may be rendered.
     *
     * Render layers will be composited to the screen as multiple ordered
     * layers when a frame is rendered.
     */
    class RenderLayer {
        friend class Renderer;

        private:
            Renderer *parent_renderer;

            const RenderItem *root_item;

            GLuint framebuffer;
            GLuint gl_texture;

            Transform transform;

            RenderLayer(Renderer *const parent);

            ~RenderLayer(void) = default;

            void render(void) const;
        
        public:
            /**
             * \brief Destroys this RenderLayer and removes it from the parent
             * renderer.
             */
            void destroy(void);
    };

    class Renderer {
        friend class Window;

        private:
            Window &window;
            SDL_GLContext gl_context;

            std::vector<RenderLayer*> render_layers;

            Renderer(Window &window);

            ~Renderer(void);

        public:
            /**
             * \brief Destroys this renderer.
             *
             * Warning: This method destroys the Renderer object. No other
             * methods should be invoked upon it after calling destroy().
             */
            void destroy(void);

            /**
             * Makes this Renderer's GL context current, such that future GL
             * calls will apply to it.
             */
            void activate_gl_context(void) const;

            /**
             * \brief Creates a new render layer with the given priority.
             *
             * Layers with higher priority will be rendered after (ergo in front
             * of) those with lower priority.
             */
            RenderLayer &create_render_layer(const int priority);

            /**
             * \brief Removes a render layer from this renderer and destroys it.
             */
            void remove_render_layer(RenderLayer &layer);
    };

}
