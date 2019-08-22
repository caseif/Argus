#pragma once

// include these first so they don't collide with the min/max macros in Windef.h
#define VMMLIB_OLD_TYPEDEFS
#include "vmmlib/matrix.hpp"
#include "vmmlib/vector.hpp"

// module lowlevel
#include "argus/threading.hpp"

// module core
#include "argus/core.hpp"

#include <atomic>
#include <functional>
#include <initializer_list>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <SDL2/SDL.h>

#define SHADER_VERTEX 1
#define SHADER_FRAGMENT 2

namespace argus {

    using vmml::vec2f;
    using vmml::vec3f;
    using vmml::vec4f;
    using vmml::mat4f;

    template <typename T>
    struct pair_t {
        T a;
        T b;
    };

    // forward declarations
    class Transform;
    class Shader;
    class ShaderProgram;
    class Window;
    class Renderer;
    class RenderLayer;
    class RenderGroup;
    class Renderable;
    class RenderableFactory;
    class RenderableTriangle;

    typedef unsigned int handle_t;
    typedef int shandle_t;

    typedef std::function<void(Window &window)> WindowCloseCallback;

    class Transform {
        private:
            vec2f translation;
            std::atomic<float> rotation;
            vec2f scale;

            std::mutex translation_mutex;
            std::mutex scale_mutex;

            bool dirty;

        public:
            Transform(void);

            // we need to explicitly define move/copy ctors to keep things atomic,
            // and also because mutexes can't be moved/copied
            Transform(Transform &rhs);

            Transform(Transform &&rhs);

            Transform(vec2f const &translation, const float rotation, vec2f const &scale);

            Transform operator +(const Transform rhs);

            vec2f const get_translation(void);

            void set_translation(vec2f const &translation);

            void add_translation(vec2f const &translation_delta);
            
            const float get_rotation(void) const;

            void set_rotation(const float rotation_degrees);

            void add_rotation(const float rotation_degrees);

            vec2f const get_scale(void);

            void set_scale(vec2f const &scale);

            void to_matrix(float (&dst_arr)[16]);

            const bool is_dirty(void) const;

            void clean(void);
    };

    struct Vertex {
        vec2f position;
        vec4f color;
        vec3f tex_coord;
    };

    class Shader {
        friend class ShaderProgram;

        private:
            const unsigned int type;
            const std::string src;
            const std::string entry_point;
            const std::vector<std::string> uniform_ids;

            Shader(const unsigned int type, std::string const &src, std::string const &entry_point,
                    std::initializer_list<std::string> const &uniform_ids);

        public:
            static Shader &create_vertex_shader(const std::string src, const std::string entry_point,
                    const std::initializer_list<std::string> &uniform_ids);

            static Shader create_vertex_shader_stack(const std::string src, const std::string entry_point,
                    const std::initializer_list<std::string> &uniform_ids);

            static Shader &create_fragment_shader(const std::string src, const std::string entry_point,
                    const std::initializer_list<std::string> &uniform_ids);

            static Shader create_fragment_shader_stack(const std::string src, const std::string entry_point,
                    const std::initializer_list<std::string> &uniform_ids);
    };

    class ShaderProgram {
        friend class RenderLayer;
        friend class RenderGroup;

        private:
            std::vector<const Shader*> shaders;
            std::vector<shandle_t> shader_handles;
            std::unordered_map<std::string, handle_t> uniforms;

            bool initialized;
            bool needs_rebuild;

            handle_t program_handle;

            ShaderProgram(const std::vector<const Shader*> &shaders);

            void link(void);

            void update_shaders(std::vector<const Shader*> &shaders);

            void update_projection_matrix(const unsigned int viewport_width, const unsigned int viewport_height);

        public:
            handle_t get_uniform_location(std::string const &uniform_id) const;
    };

    class Renderer {
        friend class Window;

        private:
            Window &window;
            std::vector<RenderLayer*> render_layers;

            SDL_GLContext gl_context;
            Index callback_id;

            bool initialized;
            std::atomic_bool destruction_pending;
            bool valid;

            Renderer(Window &window);

            Renderer(Renderer &rhs);

            Renderer(Renderer &&rhs);

            ~Renderer(void);

            void init(void);

            void render(const TimeDelta delta);

        public:
            /**
             * \brief Destroys this renderer.
             *
             * Warning: This method destroys the Renderer object. No other
             * methods should be invoked upon it after calling destroy().
             */
            void destroy(void);

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

    class Window {
        friend class Renderer;

        private:
            Renderer renderer;

            SDL_Window *handle;

            Index callback_id;
            Index listener_id;

            Window *parent;
            std::vector<Window*> children;

            struct {
                AtomicDirtiable<std::string> title;
                AtomicDirtiable<bool> fullscreen;
                AtomicDirtiable<pair_t<unsigned int>> resolution;
                AtomicDirtiable<pair_t<int>> position;
            } properties;

            WindowCloseCallback close_callback;

            std::atomic<unsigned int> state;

            Window(void);
            
            ~Window(void);

            void remove_child(Window &child);

            void update(const Timestamp delta);

            static int event_filter(void *data, SDL_Event *event);

            static void update_window(const Window &window, const Timestamp delta);

            static void window_event_callback(void *data, SDL_Event &event);

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
            Renderer &get_renderer(void);

            /**
             * Sets the window title.
             *
             * \param title The new window title.
             */
            void set_title(std::string const &title);

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
            void set_windowed_position(const int x, const int y);

            void set_close_callback(WindowCloseCallback callback);

            /*
             * \brief Activates the window.
             *
             * This function should be invoked only once.
             */
            void activate(void);
    };

    class RenderableFactory {
        friend class RenderGroup;

        private:
            RenderGroup &parent;

            RenderableFactory(RenderGroup &parent);

        public:
            RenderableTriangle &create_triangle(Vertex const &corner_1, Vertex const &corner_2, Vertex const &corner_3) const;
    };

    class RenderGroup {
        friend Renderable;
        friend RenderLayer;
        friend RenderableFactory;

        private:
            RenderLayer &parent;
            std::vector<Renderable*> children;

            Transform transform;

            std::vector<const Shader*> shaders;

            RenderableFactory renderable_factory;

            size_t vertex_count;

            bool dirty_children;
            bool dirty_shaders;

            bool shaders_initialized;
            bool buffers_initialized;

            ShaderProgram shader_program;
            handle_t vbo;
            handle_t vao;
            
            RenderGroup(RenderLayer &parent);

            ShaderProgram generate_initial_program(void);

            void update_buffer(void);

            void refresh_shaders(void);

            void draw(void);

            void add_renderable(Renderable &renderable);

            void remove_renderable(Renderable &renderable);

        public:
            void destroy(void);

            Transform &get_transform(void);

            /**
             * \brief Returns a factory for creating Renderables attached to
             * this RenderGroup.
             *
             * \returns This RenderGroup's Renderable factory.
             */
            RenderableFactory &get_renderable_factory(void);

            void add_shader(Shader const &shader);

            void remove_shader(Shader const &shader);
    };

    /**
     * Represents a layer to which geometry may be rendered.
     *
     * Render layers will be composited to the screen as multiple ordered
     * layers when a frame is rendered.
     */
    class RenderLayer {
        friend class Renderer;
        friend class RenderGroup;
        friend class RenderableFactory;

        private:
            Renderer *parent_renderer;

            std::vector<RenderGroup*> children;

            std::vector<const Shader*> shaders;

            RenderGroup root_group;

            Transform transform;

            bool dirty_shaders;

            RenderLayer(Renderer *const parent);

            ~RenderLayer(void) = default;

            void render(void);

            void remove_group(RenderGroup &group);
        
        public:
            /**
             * \brief Destroys this RenderLayer and removes it from the parent
             * renderer.
             */
            void destroy(void);

            Transform &get_transform(void);

            /**
             * \brief Returns a factory for creating Renderables attached to
             * this RenderLayer's root RenderGroup.
             *
             * \returns This RenderLayer's root Renderable factory.
             */
            RenderableFactory &get_renderable_factory(void);

            RenderGroup &create_render_group(const int priority);

            void add_shader(Shader const &shader);

            void remove_shader(Shader const &shader);
    };

    /**
     * \brief Represents an item to be rendered.
     *
     * Each item may have its own rendering properties, as well as a list of
     * child items. Child items will inherit the transform of their parent,
     * which is added to their own.
     */
    class Renderable {
        friend class RenderGroup;

        protected:
            RenderGroup &parent;

            Transform transform;

            Renderable(RenderGroup &parent);

            ~Renderable(void) = default;

            virtual void render(const handle_t buffer_handle, const size_t offset) const = 0;

            virtual const unsigned int get_vertex_count(void) const = 0;

        public:
            void remove(void);

            Transform const &get_transform(void) const;
    };

    class RenderableTriangle : Renderable {
        friend class RenderableFactory;

        private:
            const Vertex corner_1;
            const Vertex corner_2;
            const Vertex corner_3;

            RenderableTriangle(RenderGroup &parent_group, Vertex const &corner_1, Vertex const &corner_2, Vertex const &corner_3);

            void render(const handle_t buffer_handle, const size_t offset) const override;

            const unsigned int get_vertex_count(void) const override;
    };

}
