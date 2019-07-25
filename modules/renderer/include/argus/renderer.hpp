#pragma once

#include <functional>
#include <string>
#include <vector>
#include <SDL2/SDL_render.h>

#include "vmmlib/matrix.hpp"
#include "vmmlib/vector.hpp"

namespace vmml {
    typedef Vector<2, double> Vector2d;
}

namespace argus {

    class Renderer;

    class Window {
        friend class Renderer;

        private:
            SDL_Window *handle;
            Window *parent;
            std::vector<Window*> children;

            Window(void);
            
            ~Window(void);

            void remove_child(Window *child);

            void update(unsigned long long delta);

            static void update_window(Window *window, unsigned long long delta);

        public:
            /**
             * \brief Creates a new window.
             *
             * \return The new window.
             */
            static Window *create_window(void);

            /**
             * \brief Destroys this window.
             *
             * Warning: This method destroys the Window object. No other
             * methods should be invoked upon it after calling destroy().
             */
            void destroy(void);

            /**
             * \brief Creates a new renderer for this window.
             *
             * This function must be called before invoking `activate`, and must
             * not be called more than once.
             *
             * \return The new renderer.
             */
            Renderer *create_renderer(void);

            /**
             * \brief Creates a new window as a child of this one.
             *
             * Note that the child window will not be modal to the parent.
             *
             * \return The new child window.
             */
            Window *create_child_window(void);

            /**
             * Sets the window title.
             *
             * \param title The new window title.
             */
            void set_title(std::string title);

            /**
             * \brief Sets the fullscreen state of the window.
             *
             * Caution: This may not be supported on all platforms.
             *
             * \param fullscreen Whether the window is to be displayed in
             *                   fullscreen.
             */
            void set_fullscreen(bool fullscreen);

            /**
             * \brief Sets the resolution of the window when not in fullscreen
             *        mode.
             *
             * Caution: This may not be supported on all platforms.
             *
             * \param width The new width of the window.
             * \param height The new height of the window.
             */
            void set_resolution(unsigned int width, unsigned int height);

            /**
             * \brief Sets the position of the window on the screen when in
             *        windowed mode.
             *
             * Caution: This may not be supported on all platforms.
             *
             * \param x The new X-coordinate of the window.
             * \param y The new Y-coordinate of the window.
             */
            void set_windowed_position(unsigned int x, unsigned int y);

            /*
             * \brief Activates the window.
             *
             * This function should be invoked only once.
             */
            void activate(void);
    };

    class Renderer {
        friend class Window;

        private:
            SDL_Renderer *handle;

            Renderer(Window *window);

            ~Renderer(void);

        public:
            /**
             * \brief Destroys this renderer.
             *
             * Warning: This method destroys the Renderer object. No other
             * methods should be invoked upon it after calling destroy().
             */
            void destroy(void);
    };

    class Transform {
        private:
            Transform *parent;
            vmml::Vector2d translation;
            double rotation;
            vmml::Vector2d scale;

        public:
            Transform(void);

            Transform(vmml::Vector2d translation, double rotation, vmml::Vector2d scale);

            ~Transform(void);

            vmml::Vector2d get_translation(void) const;

            void set_translation(const vmml::Vector2d &translation);

            void add_translation(const vmml::Vector2d &translation_delta);
            
            double get_rotation(void) const;

            void set_rotation(double rotation_degrees);

            void add_rotation(double rotation_degrees);

            vmml::Vector2d get_scale(void) const;

            void set_scale(const vmml::Vector2d &scale);

            void set_parent(const Transform &transform);

            const vmml::Matrix3d &to_matrix(void);
    };

    /**
     * Represents an object to be rendered to the screen in some capacity.
     *
     * Note that a render target may be rendered to another RenderObject before
     * ultimately being rasterized to the screen.
     */
    class RenderObject {
        protected:
            SDL_Texture *handle;

            Transform transform;

            RenderObject(void);

            ~RenderObject(void) = default;

            void queue_render_object(RenderObject);

        public:
            /**
             * Queues this object to be rendered to another RenderObject with
             * its current transformation.
             */
            void render_to(RenderObject&);
    };

    /**
     * Represents a layer to be rendered to the screen.
     */
    class RenderLayer : RenderObject {
        private:
            RenderLayer(void);

            ~RenderLayer(void) = default;

        public:
            void destroy(void);
    };

}
