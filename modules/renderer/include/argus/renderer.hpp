#pragma once

// module lowlevel
#include "argus/math.hpp"
#include "argus/threading.hpp"

// module core
#include "argus/core.hpp"

// module resman
#include "argus/resource_manager.hpp"

#include <atomic>
#include <functional>
#include <initializer_list>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#define SHADER_VERTEX 1
#define SHADER_FRAGMENT 2

namespace argus {

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
    class RenderableSquare;

    typedef unsigned int handle_t;
    typedef int shandle_t;

    typedef void *window_handle_t;
    typedef void *graphics_context_t;
    typedef void *graphics_handle_t;

    typedef std::function<void(Window &window)> WindowCloseCallback;

    class Transform {
        private:
            Vector2f translation;
            std::atomic<float> rotation;
            Vector2f scale;

            std::mutex translation_mutex;
            std::mutex scale_mutex;

            bool dirty;

        public:
            Transform(void);

            // we need to explicitly define move/copy ctors to keep things atomic,
            // and also because mutexes can't be moved/copied
            Transform(Transform &rhs);

            Transform(Transform &&rhs);

            Transform(Vector2f const &translation, const float rotation, Vector2f const &scale);

            Transform operator +(const Transform rhs);

            Vector2f const get_translation(void);

            void set_translation(Vector2f const &translation);

            void add_translation(Vector2f const &translation_delta);
            
            const float get_rotation(void) const;

            void set_rotation(const float rotation_degrees);

            void add_rotation(const float rotation_degrees);

            Vector2f const get_scale(void);

            void set_scale(Vector2f const &scale);

            void to_matrix(float (&dst_arr)[16]);

            const bool is_dirty(void) const;

            void clean(void);
    };

    struct Vertex {
        Vector2f position;
        Vector4f color;
        Vector2f tex_coord;
    };

    struct TextureData {
        private:            
            std::atomic_bool prepared;

        public:
            const size_t width;
            const size_t height;
            const size_t bpp;
            const size_t channels;
            unsigned char **image_data;
            handle_t buffer_handle;

        TextureData(const size_t width, const size_t height, const size_t bpp, const size_t channels,
                unsigned char **image_data);

        ~TextureData(void);

        const size_t get_data_length(void);

        const bool is_prepared(void);

        void prepare(void);
        
        const unsigned int get_pixel_format(void) const;
    };

    class Shader {
        friend class ShaderProgram;

        private:
            const unsigned int type;
            const std::string src;
            const std::string entry_point;
            const int priority;
            const std::vector<std::string> uniform_ids;

            Shader(const unsigned int type, std::string const &src, std::string const &entry_point,
                    const int priority, std::initializer_list<std::string> const &uniform_ids);

        public:
            static Shader &create_vertex_shader(const std::string src, const std::string entry_point,
                    const int priority, const std::initializer_list<std::string> &uniform_ids);

            static Shader create_vertex_shader_stack(const std::string src, const std::string entry_point,
                    const int priority, const std::initializer_list<std::string> &uniform_ids);

            static Shader &create_fragment_shader(const std::string src, const std::string entry_point,
                    const int priority, const std::initializer_list<std::string> &uniform_ids);

            static Shader create_fragment_shader_stack(const std::string src, const std::string entry_point,
                    const int priority, const std::initializer_list<std::string> &uniform_ids);
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

            ShaderProgram(const std::vector<const Shader*> &&shaders);

            void link(void);

            void update_shaders(const std::vector<const Shader*> &shaders);

            void update_projection_matrix(const unsigned int viewport_width, const unsigned int viewport_height);

        public:
            handle_t get_uniform_location(std::string const &uniform_id) const;
    };

    class Renderer {
        friend class Window;

        private:
            Window &window;
            std::vector<RenderLayer*> render_layers;

            graphics_context_t gl_context;
            Index callback_id;

            bool initialized;
            std::atomic_bool destruction_pending;
            bool valid;

            std::atomic_bool dirty_resolution;

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

            window_handle_t handle;

            Index callback_id;
            Index listener_id;

            Window *parent;
            std::vector<Window*> children;

            struct {
                AtomicDirtiable<std::string> title;
                AtomicDirtiable<bool> fullscreen;
                AtomicDirtiable<Vector2u> resolution;
                AtomicDirtiable<Vector2i> position;
            } properties;

            WindowCloseCallback close_callback;

            std::atomic<unsigned int> state;

            Window(void);
            
            ~Window(void);

            void remove_child(Window &child);

            void update(const Timestamp delta);

            static void update_window(const Window &window, const Timestamp delta);

            static bool event_filter(ArgusEvent &event, void *user_data);

            static void event_callback(ArgusEvent &event, void *user_data);

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
            
            RenderableSquare &create_square(Vertex const &corner_1, Vertex const &corner_2,
                    Vertex const &corner_3, Vertex const &corner_4) const;
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

            std::map<std::string, unsigned int> texture_indices;

            RenderableFactory renderable_factory;

            size_t vertex_count;

            bool dirty_children;
            bool dirty_shaders;

            bool shaders_initialized;
            bool buffers_initialized;

            ShaderProgram shader_program;
            handle_t vbo;
            handle_t vao;
            handle_t tex_handle;
            
            RenderGroup(RenderLayer &parent);

            ShaderProgram generate_initial_program(void);

            void rebuild_textures(bool force);

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
            Renderer &parent_renderer;
            const int priority;

            std::vector<RenderGroup*> children;
            std::vector<const Shader*> shaders;

            RenderGroup root_group; // this depends on shaders being initialized

            Transform transform;

            bool dirty_shaders;

            RenderLayer(Renderer &parent, const int priority);

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

        private:
            void release_texture();

            float *vertex_buffer;
            size_t buffer_head;
            size_t buffer_size;
            size_t max_buffer_size;

            handle_t tex_buffer;
            unsigned int tex_index;
            Vector2f tex_max_uv;
            std::atomic_bool dirty_texture;

        protected:
            RenderGroup &parent;

            Transform transform;

            Resource *tex_resource;

            Renderable(RenderGroup &parent);

            ~Renderable(void);

            void allocate_buffer(const size_t vertex_count);

            void buffer_vertex(Vertex const &vertex);

            virtual void populate_buffer(void) = 0;

            virtual const unsigned int get_vertex_count(void) const = 0;

        public:
            void remove(void);

            Transform const &get_transform(void) const;

            void set_texture(std::string const &texture_uid);
    };

    class RenderableTriangle : public Renderable {
        friend class RenderableFactory;

        private:
            const Vertex corner_1;
            const Vertex corner_2;
            const Vertex corner_3;

            RenderableTriangle(RenderGroup &parent_group, Vertex const &corner_1, Vertex const &corner_2,
                    Vertex const &corner_3);

            void populate_buffer(void) override;

            const unsigned int get_vertex_count(void) const override;
    };

    class RenderableSquare : public Renderable {
        friend class RenderableFactory;

        private:
            const Vertex corner_1;
            const Vertex corner_2;
            const Vertex corner_3;
            const Vertex corner_4;

            RenderableSquare(RenderGroup &parent_group, Vertex const &corner_1, Vertex const &corner_2,
                    Vertex const &corner_3, Vertex const &corner_4);

            void populate_buffer(void) override;

            const unsigned int get_vertex_count(void) const override;
    };

}
