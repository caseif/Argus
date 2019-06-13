#pragma once

#include <string>
#include <SDL2/SDL_render.h>

namespace argus {

    class Renderer;

    class Window {
        friend class Renderer;

        private:
            SDL_Window *handle;

            Window(void);

        public:
            /* Creates a new window.                                              */
            static Window *create_window(void);

            /* Creates a new renderer for this window.                            */
            Renderer *create_renderer(void);

            /* Sets the window title.                                             */
            void set_title(std::string title);

            /* Sets the fullscreen state of the window.                           */
            /* Caution: This may not be supported on all platforms.               */
            void set_fullscreen(bool fullscreen);

            /* Sets the resolution of the window when not in fullscreen mode.     */
            /* Caution: This may not be supported on all platforms.               */
            void set_windowed_resolution(unsigned int width, unsigned int height);

            /* Sets the position of the window on the screen when not in          */
            /* fullscreen mode.                                                   */
            /* Caution: This may not be supported on all platforms.               */
            void set_windowed_position(unsigned int x, unsigned int y);

            /* Activates the window. This function should be invoked only once.   */
            void activate(void);

            /* Returns the underlying SDL window to allow low-level control. If   */
            /* the window is not yet active, this will return a null pointer.     */
            /* Caution: Use of this function is generally not recommended, as     */
            /* Argus provides a higher-level abstraction for most functionality.  */
            SDL_Window *get_sdl_window(void);
    };

    class Renderer {
        friend class Window;

        private:
            SDL_Renderer *handle;

            Renderer(Window *window);

        public:
            /* Returns the underlying SDL renderer to allow low-level control. If */
            /* the window is not yet active, this will return a null pointer.     */
            /* Caution: Use of this function is generally not recommended, as     */
            /* Argus provides a higher-level abstraction for most functionality.  */
            SDL_Renderer *get_sdl_renderer(void);
    };

}
