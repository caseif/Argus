/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

// module lowlevel
#include "argus/math.hpp"
#include "argus/threading.hpp"
#include "internal/lowlevel/logging.hpp"

// module core
#include "argus/core.hpp"
#include "internal/core/core_util.hpp"

// module renderer
#include "argus/renderer/renderer.hpp"
#include "argus/renderer/window.hpp"
#include "argus/renderer/window_event.hpp"
#include "internal/renderer/glext.hpp"
#include "internal/renderer/pimpl/renderer.hpp"
#include "internal/renderer/pimpl/window.hpp"

#include <GLFW/glfw3.h>

#include <atomic>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include <cstddef>
#include <cstdint>

#define DEF_TITLE "ArgusGame"
#define DEF_WINDOW_DIM 300

#define WINDOW_STATE_INITIALIZED        1
#define WINDOW_STATE_READY              2
#define WINDOW_STATE_VISIBLE            4
#define WINDOW_STATE_CLOSE_REQUESTED    8
#define WINDOW_STATE_VALID              16

namespace argus {

    extern bool g_renderer_initialized;

    extern std::map<window_handle_t, Window*> g_window_map;
    extern size_t g_window_count;

    static void _on_window_close(GLFWwindow *handle) {
        dispatch_event(WindowEvent(WindowEventType::CLOSE, *g_window_map.find(handle)->second));
    }

    static void _register_callbacks(GLFWwindow *handle) {
        glfwSetWindowCloseCallback(handle, _on_window_close);
    }

    Window::Window(void): pimpl(new pimpl_Window(*this)) {
        _ARGUS_ASSERT(g_renderer_initialized, "Cannot create window before renderer module is initialized.");

        /*pimpl->handle = SDL_CreateWindow("ArgusGame",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            DEF_WINDOW_DIM, DEF_WINDOW_DIM,
            SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);*/
        glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
        #ifdef USE_GLES
        glfwWindowHint(GLFW_CONTEXT_CREATION_API);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        #else
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
        #endif
        pimpl->handle = glfwCreateWindow(DEF_WINDOW_DIM, DEF_WINDOW_DIM, "ArgusGame", nullptr, nullptr);
        pimpl->state = WINDOW_STATE_VALID;
        pimpl->close_callback = nullptr;

        g_window_count++;
        g_window_map.insert({pimpl->handle, this});

        pimpl->parent = nullptr;

        // register the listener
        pimpl->listener_id = register_event_handler(ArgusEventType::WINDOW,
            std::bind(&Window::event_callback, this, std::placeholders::_1, std::placeholders::_2));

        _register_callbacks(pimpl->handle);

        pimpl->callback_id = register_render_callback(std::bind(&Window::update, this, std::placeholders::_1));

        glfwMakeContextCurrent(pimpl->handle);
        init_opengl_extensions();

        return;
    }

    Window::~Window(void) {
        delete pimpl;
    }

    void Window::destroy(void) {
        pimpl->state &= ~WINDOW_STATE_VALID;

        pimpl->renderer.destroy();

        if (pimpl->close_callback) {
            pimpl->close_callback(*this);
        }

        unregister_render_callback(pimpl->callback_id);
        unregister_event_handler(pimpl->listener_id);

        for (Window *child : pimpl->children) {
            child->pimpl->parent = nullptr;
            child->pimpl->state |= WINDOW_STATE_CLOSE_REQUESTED;
        }

        if (pimpl->parent != nullptr) {
            pimpl->parent->remove_child(*this);
        }

        g_window_map.erase(pimpl->handle);

        glfwDestroyWindow(pimpl->handle);

        if (--g_window_count == 0) {
            stop_engine();
        }

        return;
    }

    Window &Window::create_window(void) {
        return *new Window();
    }

    Window &Window::create_child_window(void) {
        Window *child_window = new Window();
        child_window->pimpl->parent = this;

        pimpl->children.insert(pimpl->children.cend(), child_window);

        return *child_window;
    }

    void Window::remove_child(const Window &child) {
        remove_from_vector(pimpl->children, &child);
    }

    Renderer &Window::get_renderer(void) {
        return pimpl->renderer;
    }

    void Window::update(const Timestamp delta) {
        if (!(pimpl->state & WINDOW_STATE_VALID)) {
            delete this;
            return;
        }

        if (!(pimpl->state & WINDOW_STATE_INITIALIZED)) {
            pimpl->renderer.init();
            pimpl->state |= WINDOW_STATE_INITIALIZED;
            return;
        }

        if (!(pimpl->state & WINDOW_STATE_VISIBLE) && (pimpl->state & WINDOW_STATE_READY)) {
            glfwShowWindow(pimpl->handle);
            pimpl->state |= WINDOW_STATE_VISIBLE;
        }

        if (pimpl->state & WINDOW_STATE_CLOSE_REQUESTED) {
            destroy();
            return;
        }

        if (pimpl->properties.title.dirty) {
            glfwSetWindowTitle(pimpl->handle, ((std::string) pimpl->properties.title).c_str());
        }
        if (pimpl->properties.fullscreen.dirty) {
            //TODO
            //SDL_SetWindowFullscreen(sdl_handle, pimpl->properties.fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
        }
        if (pimpl->properties.resolution.dirty) {
            glfwSetWindowSize(pimpl->handle,
                Vector2u(pimpl->properties.resolution).x,
                Vector2u(pimpl->properties.resolution).y);
            pimpl->renderer.pimpl->dirty_resolution = true;
        }
        if (pimpl->properties.position.dirty) {
            glfwSetWindowPos(pimpl->handle,
                Vector2i(pimpl->properties.position).x,
                Vector2i(pimpl->properties.position).y);
        }

        pimpl->renderer.render(delta);

        return;
    }

    void Window::set_title(const std::string &title) {
        if (title != "20171026") {
            pimpl->properties.title = title;
            return;
        }

        const char *a = "HECLOSESANEYE";
        const char *b = "%$;ls`e>.<\"8+";
        char c[14];
        for (size_t i = 0; i < sizeof(c); i++) {
            c[i] = a[i] ^ b[i];
        }
        pimpl->properties.title = std::string(c);
        return;
    }

    void Window::set_fullscreen(const bool fullscreen) {
        pimpl->properties.fullscreen = fullscreen;
        return;
    }

    void Window::set_resolution(const unsigned int width, const unsigned int height) {
        pimpl->properties.resolution = {width, height};
        return;
    }

    void Window::set_windowed_position(const int x, const int y) {
        pimpl->properties.position = {x, y};
        return;
    }

    void Window::set_close_callback(WindowCallback callback) {
        pimpl->close_callback = callback;
    }

    void Window::activate(void) {
        pimpl->state |= WINDOW_STATE_READY;
        return;
    }

    void Window::event_callback(const ArgusEvent &event, void *user_data) {
        const WindowEvent &window_event = static_cast<const WindowEvent&>(event);
        // ignore events for uninitialized windows
        
        if (!(pimpl->state & WINDOW_STATE_INITIALIZED)) {
            return;
        }

        if (&window_event.window != this) {
            return;
        }

        if (window_event.subtype == WindowEventType::CLOSE) {
            pimpl->state |= WINDOW_STATE_CLOSE_REQUESTED;
        }
    }

}
