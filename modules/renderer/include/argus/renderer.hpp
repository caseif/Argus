#pragma once

#include <functional>
#include <string>
#include <vector>
#include <SDL2/SDL_render.h>

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

            /**
             * \brief Returns the underlying SDL window to allow low-level
             *        control.
             *
             * Caution: Use of this function is generally not recommended, as
             * Argus provides a higher-level abstraction for most typical
             * functionality.
             *
             * \return The underlying SDL_window.
             */
            SDL_Window *get_sdl_window(void);
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

            /**
             * \brief Returns the underlying SDL renderer to allow low-level
             *        control.
             *
             * Caution: Use of this function is generally not recommended, as
             * Argus provides a higher-level abstraction for most functionality.
             *
             * \return The underlying SDL renderer
             */
            SDL_Renderer *get_sdl_renderer(void);
    };

}
