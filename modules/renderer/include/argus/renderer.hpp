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

    /**
     * \brief An unsigned integer handle to an object.
     */
    typedef unsigned int handle_t;
    /**
     * \brief A signed integer handle to an object.
     */
    typedef int shandle_t;

    /**
     * \brief A handle to a window.
     */
    typedef void *window_handle_t;
    /**
     * \brief A handle to a graphics context.
     */
    typedef void *graphics_context_t;

    /**
     * \brief A callback which operates on a window-wise basis.
     */
    typedef std::function<void(Window &window)> WindowCallback;

    enum class WindowEventType {
        CLOSE,
        MINIMIZE,
        RESTORE
    };

    struct WindowEvent : public ArgusEvent {
        const WindowEventType subtype;
        const Window *window;

        WindowEvent(const WindowEventType subtype, Window *window):
                ArgusEvent{ArgusEventType::WINDOW},
                subtype(subtype),
                window(window) {
        }
    };

    /**
     * \brief A transformation in 2D space.
     *
     * All member functions of this class are thread-safe.
     */
    class Transform {
        private:
            Vector2f translation;
            std::atomic<float> rotation;
            Vector2f scale;

            std::mutex translation_mutex;
            std::mutex scale_mutex;

            std::atomic_bool dirty;

        public:
            Transform(void);

            // we need to explicitly define move/copy ctors to keep things atomic,
            // and also because mutexes can't be moved/copied
            Transform(Transform &rhs);

            Transform(Transform &&rhs);

            /**
             * \brief Constructs a new 2D Transform with the given parameters.
             */
            Transform(Vector2f const &translation, const float rotation, Vector2f const &scale);

            /**
             * \brief Adds one Transform to another.
             *
             * The translation and rotation combinations are additive, while the
             * scale combination is multiplicative.
             */
            Transform operator +(const Transform rhs);

            /**
             * \brief Gets the translation component of this Transform.
             *
             * \return The translation component of this Transform.
             */
            Vector2f const get_translation(void);

            /**
             * \brief Sets the translation component of this Transform.
             *
             * \param translation The new translation for this Transform.
             */
            void set_translation(Vector2f const &translation);

            /**
             * \brief Adds the given value to this Transform's translation
             * component.
             *
             * \param translation_delta The value to add to this Transform's
             *         translation component.
             */
            void add_translation(Vector2f const &translation_delta);

            /**
             * \brief Gets the rotation component of this Transform in radians.
             *
             * \return The rotation component of this Transform in radians.
             */
            const float get_rotation(void) const;

            /**
             * \brief Sets the rotation component of this Transform.
             *
             * \prarm rotation_radians The new rotation component for this Transform.
             */
            void set_rotation(const float rotation_radians);

            /**
             * \brief Adds the given value to this Transform's rotation
             * component.
             *
             * \param rotation_radians The value in radians to add to this
             *         Transform's rotation component.
             */
            void add_rotation(const float rotation_radians);

            /**
             * \brief Gets the scale component of this Transform.
             *
             * \return The scale component of this Transform.
             */
            Vector2f const get_scale(void);

            /**
             * \brief Sets the scale component of this Transform.
             *
             * \param scale The new scale component for this Transform.
             */
            void set_scale(Vector2f const &scale);

            /**
             * \brief Generates a 4x4 matrix in column-major form representing
             * this Transform and stores it in the given parameter.
             *
             * \param dst_arr A pointer to the location where the matrix data
             *         will be stored.
             */
            void to_matrix(float (&dst_arr)[16]);

            /**
             * \brief Gets whether this transform has been modified since the
             * last time the clean() member function was invoked.
             *
             * \return Whether this transform is dirty.
             */
            const bool is_dirty(void) const;

            /**
             * Unsets this Transform's dirty flag.
             */
            void clean(void);
    };

    /**
     * \brief Represents a vertex in 2D space containing a 2-dimensional spatial
     * position, an RGBA color value, and 2-dimensional texture UV coordinates.
     */
    struct Vertex {
        /**
         * \brief The position of this vertex in 2D space.
         */
        Vector2f position;
        /**
         * \brief The RGBA color of this vertex in [0,1] space.
         */
        Vector4f color;
        /**
         * \brief The texture coordinates of this vertex in UV-space.
         */
        Vector2f tex_coord;
    };

    /**
     * \brief Contains metadata and data pertaining to an image to be used as a
     * texture for rendering.
     *
     * Depending on whether the data has been prepared by the renderer, the
     * object may or may not contain the image data. Image data is deleted
     * after it has been uploaded to the GPU during texture preparation.
     */
    struct TextureData {
        private:
            /**
             * \brief A two-dimensional array of pixel data for this texture.
             *
             * The data is stored in column-major form. If the texture has
             * already been prepared for use in rendering, the data will no
             * longer be present in system memory and the pointer will be
             * equivalent to nullptr.
             */
            unsigned char **image_data;

            /**
             * \brief Whether the texture data has been prepared for use.
             */
            std::atomic_bool prepared;

        public:
            /**
             * \brief The width in pixels of the texture.
             */
            const size_t width;
            /**
             * \brief The height in pixels of the texture.
             */
            const size_t height;
            /**
             * \brief A handle to the buffer in video memory storing this
             * texture's data. This handle is only valid after the texture data
             * has been prepared for use.
             */
            handle_t buffer_handle;

        /**
         * Constructs a new instance of this class with the given metadata and
         * pixel data.
         *
         * The pixel data must be in RGBA format with a bit-depth of 8.
         *
         * \param width The width of the texture in pixels.
         * \param height The height of the texture in pixels.
         * \param image_data A pointer to a column-major 2D-array containing the
         *         texture's pixel data. This *must* point to heap memory. The
         *         calling method's copy of the pointer will be set to nullptr.
         */
        TextureData(const size_t width, const size_t height, unsigned char **&&image_data);

        /**
         * \brief Destroys this object, deleting any buffers in system or video
         * memory currently in use.
         */
        ~TextureData(void);

        /**
         * \brief Gets whether the texture data has been prepared for use in
         * rendering.
         *
         * \return Whether the texture data has been prepared.
         */
        const bool is_prepared(void);

        /**
         * \brief Prepares the texture data for use in rendering.
         */
        void prepare(void);
    };

    /**
     * \brief Represents a shader for use with a RenderGroup or RenderLayer.
     *
     * Because of limitations in the low-level API, Argus requires that each
     * shader specify an entry point other than main(). When shaders are built,
     * a main() function is generated containing calls to each shader's
     * respective entry point.
     */
    class Shader {
        friend class ShaderProgram;

        private:
            /**
             * \brief The type of this shader as a magic value.
             */
            const unsigned int type;
            /**
             * \brief The source code of this shader.
             */
            const std::string src;
            /**
             * \brief The name of this shader's entry point.
             */
            const std::string entry_point;
            /**
             * \brief The priority of this shader.
             * 
             * Higher priority shaders will be processed before lower priority
             * ones within their respective stage.
             */
            const int priority;
            /**
             * \brief The uniforms defined by this shader.
             */
            const std::vector<std::string> uniform_ids;

            /**
             * \brief Constructs a new Shader with the given parameters.
             *
             * \param type The type of the Shader as a magic value.
             * \param src The source code of the Shader.
             * \param entry_point The entry point of the Shader.
             * \param priority The priority of the Shader.
             * \param uniform_ids A list of uniforms defined by this shader.
             */
            Shader(const unsigned int type, std::string const &src, std::string const &entry_point,
                    const int priority, std::initializer_list<std::string> const &uniform_ids);

        public:
            /**
             * \brief Creates a new vertex shader on the heap with the given parameters.
             *
             * \param src The source code of the Shader.
             * \param entry_point The entry point of the Shader.
             * \param priority The priority of the Shader.
             * \param uniform_ids A list of uniforms defined by this shader.
             */
            static Shader &create_vertex_shader(const std::string src, const std::string entry_point,
                    const int priority, const std::initializer_list<std::string> &uniform_ids);

            /**
             * \brief Creates a new vertex shader on the stack with the given parameters.
             *
             * \param src The source code of the Shader.
             * \param entry_point The entry point of the Shader.
             * \param priority The priority of the Shader.
             * \param uniform_ids A list of uniforms defined by this shader.
             */
            static Shader create_vertex_shader_stack(const std::string src, const std::string entry_point,
                    const int priority, const std::initializer_list<std::string> &uniform_ids);

            /**
             * \brief Creates a new fragment shader on the heap with the given parameters.
             *
             * \param src The source code of the Shader.
             * \param entry_point The entry point of the Shader.
             * \param priority The priority of the Shader.
             * \param uniform_ids A list of uniforms defined by this shader.
             */
            static Shader &create_fragment_shader(const std::string src, const std::string entry_point,
                    const int priority, const std::initializer_list<std::string> &uniform_ids);

            /**
             * \brief Creates a new fragment shader on the stack with the given parameters.
             *
             * \param src The source code of the Shader.
             * \param entry_point The entry point of the Shader.
             * \param priority The priority of the Shader.
             * \param uniform_ids A list of uniforms defined by this shader.
             */
            static Shader create_fragment_shader_stack(const std::string src, const std::string entry_point,
                    const int priority, const std::initializer_list<std::string> &uniform_ids);
    };

    /**
     * \brief Represents a linked shader program for use with a RenderGroup.
     */
    class ShaderProgram {
        friend class RenderLayer;
        friend class RenderGroup;

        private:
            /**
             * \brief The list of Shaders encompassed by this program.
             */
            std::vector<const Shader*> shaders;
            /**
             * \brief uniforms A complete list of uniforms defined by this
             * program's Shaders.
             */
            std::unordered_map<std::string, handle_t> uniforms;

            /**
             * \brief Whether this program has been initially compiled and linked.
             */
            bool initialized;
            /**
             * \brief Whether this program must be rebuilt (due to the Shader
             * list updating).
             */
            bool needs_rebuild;

            /**
             * A handle to the linked program in video memory.
             */
            handle_t program_handle;

            /**
             * \brief Constructs a new ShaderProgram encompassing the given
             * Shaders.
             */
            ShaderProgram(const std::vector<const Shader*> &shaders);

            /**
             * \brief Constructs a new ShaderProgram encompassing the given
             * Shaders.
             */
            ShaderProgram(const std::vector<const Shader*> &&shaders);

            /**
             * \brief Compiles and links this program so it may be used in
             * rendering.
             */
            void link(void);

            /**
             * \brief Updates the list of Shaders encompassed by this program.
             *
             * \param shaders The new list of Shaders for this program.
             */
            void update_shaders(const std::vector<const Shader*> &shaders);

            /**
             * \brief Updates this program's implicit projection matrix uniform
             * to match the given dimensions.
             */
            void update_projection_matrix(const unsigned int viewport_width, const unsigned int viewport_height);

        public:
            /**
             * \brief Gets a handle to a given uniform defined by this program.
             *
             * \deprecated This will be removed after functions are added to
             * abstract the setting of uniforms.
             */
            handle_t get_uniform_location(std::string const &uniform_id) const;

            //TODO: uniform set abstraction
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

            WindowCallback close_callback;

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

            void set_close_callback(WindowCallback callback);

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
