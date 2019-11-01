/*
 * This file is a part of Argus.
 * Copyright (c) 2019, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

/**
 * \file argus/renderer.hpp
 *
 * Primary rendering engine interface, providing high-level abstractions for
 * rendering concepts.
 */

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
#include <set>
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

    /**
     * \brief A type of WindowEvent.
     *
     * \sa WindowEvent
     */
    enum class WindowEventType {
        CLOSE,
        MINIMIZE,
        RESTORE
    };

    /**
     * \brief An ArgusEvent pertaining to a Window.
     *
     * \sa ArgusEvent
     * \sa Window
     */
    struct WindowEvent : public ArgusEvent {
        /**
         * \brief The specific \link WindowEventType type \endlink of
         *        WindowEvent.
         */
        const WindowEventType subtype;
        /**
         * \brief The Window associated with the event.
         */
        const Window *window;

        /**
         * \brief Constructs a new WindowEvent.
         *
         * \param subtype The specific \link WindowEventType type \endlink of
         *        WindowEvent.
         * \param window The Window associated with the event.
         */
        WindowEvent(const WindowEventType subtype, Window *window):
                ArgusEvent{ArgusEventType::WINDOW},
                subtype(subtype),
                window(window) {
        }
    };

    /**
     * \brief A transformation in 2D space.
     *
     * \remark All member functions of this class are thread-safe.
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
            /**
             * \brief Constructs a Transform with no translation or rotation and
             *        1x scaling.
             */
            Transform(void);

            // we need to explicitly define move/copy ctors to keep things atomic,
            // and also because mutexes can't be moved/copied

            /**
             * \brief The copy constructor.
             *
             * \param rhs The Transform to copy.
             */
            Transform(Transform &rhs);

            /**
             * \brief The move constructor.
             *
             * \param rhs The Transform to move.
             */
            Transform(Transform &&rhs);

            /**
             * \brief Constructs a new 2D Transform with the given parameters.
             *
             * \param translation The translation in 2D space.
             * \param rotation The single-axis rotation.
             * \param scale The scale in 2D space.
             */
            Transform(const Vector2f &translation, const float rotation, const Vector2f &scale);

            /**
             * \brief Adds one Transform to another.
             *
             * The translation and rotation combinations are additive, while the
             * scale combination is multiplicative.
             *
             * \param rhs The Transform to add.
             *
             * \return The resulting Transform.
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
            void set_translation(const Vector2f &translation);

            /**
             * \brief Adds the given value to this Transform's translation
             * component.
             *
             * \param translation_delta The value to add to this Transform's
             *         translation component.
             */
            void add_translation(const Vector2f &translation_delta);

            /**
             * \brief Gets the rotation component of this Transform in radians.
             *
             * \return The rotation component of this Transform in radians.
             */
            const float get_rotation(void) const;

            /**
             * \brief Sets the rotation component of this Transform.
             *
             * \param rotation_radians The new rotation component for this Transform.
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
            void set_scale(const Vector2f &scale);

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
             * \brief Unsets this Transform's dirty flag.
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
             *        texture's data. This handle is only valid after the
             *        texture data has been prepared for use.
             */
            handle_t buffer_handle;

        /**
         * \brief Constructs a new instance of this class with the given
         *        metadata and pixel data.
         *
         * \param width The width of the texture in pixels.
         * \param height The height of the texture in pixels.
         * \param image_data A pointer to a column-major 2D-array containing the
         *        texture's pixel data. This *must* point to heap memory. The
         *        calling method's copy of the pointer will be set to nullptr.
         *
         * \attention The pixel data must be in RGBA format with a bit-depth of 8.
         */
        TextureData(const size_t width, const size_t height, unsigned char **&&image_data);

        /**
         * \brief Destroys this object, deleting any buffers in system or video
         *        memory currently in use.
         */
        ~TextureData(void);

        /**
         * \brief Gets whether the texture data has been prepared for use in
         *        rendering.
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
     * Because of limitations in the low-level graphics API, Argus requires that
     * each shader specify an entry point other than main(). When shaders are
     * built, a main() function is generated containing calls to each shader's
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
            Shader(const unsigned int type, const std::string &src, const std::string &entry_point,
                    const int priority, std::initializer_list<std::string>const  &uniform_ids);

        public:
            /**
             * \brief Creates a new vertex shader on the heap with the given parameters.
             *
             * \param src The source code of the Shader.
             * \param entry_point The entry point of the Shader.
             * \param priority The priority of the Shader.
             * \param uniform_ids A list of uniforms defined by this shader.
             *
             * \return The constructed vertex Shader.
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
             *
             * \return The constructed vertex Shader.
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
             *
             * \return The constructed fragment Shader.
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
             *
             * \return The constructed fragment Shader.
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
             * \brief The set of Shaders encompassed by this program.
             */
            std::set<const Shader*, bool(*)(const Shader*, const Shader*)> shaders;
            /**
             * \brief A complete list of uniforms defined by this
             *        program's Shaders.
             */
            std::unordered_map<std::string, handle_t> uniforms;

            /**
             * \brief Whether this program has been initially compiled and linked.
             */
            bool initialized;
            /**
             * \brief Whether this program must be rebuilt (due to the Shader
             *        list updating).
             */
            bool needs_rebuild;

            /**
             * \brief A handle to the linked program in video memory.
             */
            handle_t program_handle;

            /**
             * \brief Constructs a new ShaderProgram encompassing the given
             *        Shaders.
             *
             * \param shaders The \link Shader Shaders \endlink to construct the
             *        new program with.
             */
            ShaderProgram(const std::vector<const Shader*> &shaders);

            /**
             * \brief Compiles and links this program so it may be used in
             *        rendering.
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
             *        to match the given dimensions.
             *
             * \param viewport_width The new width of the viewport.
             * \param viewport_height The new height of the viewport.
             */
            void update_projection_matrix(const unsigned int viewport_width, const unsigned int viewport_height);

        public:
            /**
             * \brief Gets a handle to a given uniform defined by this program.
             *
             * \param uniform_id The ID string of the uniform to look up.
             *
             * \return The location of the uniform.
             *
             * \warning Invoking this method with a non-present uniform ID will
             *          trigger a fatal engine error.
             *
             * \deprecated This will be removed after functions are added to
             *             abstract the setting of uniforms.
             */
            handle_t get_uniform_location(const std::string &uniform_id) const;

            //TODO: uniform set abstraction
    };

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

        private:
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

    /**
     * \brief Represents an individual window on the screen.
     *
     * \attention Not all platforms may support multiple windows.
     *
     * \sa Renderer
     */
    class Window {
        friend class Renderer;

        private:
            /**
             * \brief The Renderer associated with this Window.
             */
            Renderer renderer;

            /**
             * \brief A handle to the lower-level window represented by this
             *        object.
             */
            window_handle_t handle;

            /**
             * \brief The ID of the engine callback registered for this Window.
             */
            Index callback_id;

            /**
             * \brief The ID of the event listener registered for this Window.
             */
            Index listener_id;

            /**
             * \brief The Window parent to this one, if applicable.
             */
            Window *parent;
            /**
             * \brief This Window's child \link Window Windows \endlink, if any.
             */
            std::vector<Window*> children;

            struct {
                AtomicDirtiable<std::string> title;
                AtomicDirtiable<bool> fullscreen;
                AtomicDirtiable<Vector2u> resolution;
                AtomicDirtiable<Vector2i> position;
            } properties;

            /**
             * \brief The callback to be executed upon the Window being closed.
             */
            WindowCallback close_callback;

            /**
             * \brief The state of this Window as a bitfield.
             *
             * \warning This field's semantic meaning is implementation-defined.
             */
            std::atomic<unsigned int> state;

            /**
             * \brief The nullary and primary constructor.
             *
             * \remark A Renderer will be implicitly created upon construction
             *         of a Window.
             */
            Window(void);

            ~Window(void);

            /**
             * \brief Removes the given Window from this Window's child list.
             *
             * \param child The child Window to remove.
             *
             * \attention This method does not alter the state of the child
             *         Window, which must be dissociated from its parent separately.
             */
            void remove_child(const Window &child);

            /**
             * \brief The primary update callback for a Window.
             *
             * \param delta The time in microseconds since the last frame.
             */
            void update(const Timestamp delta);

            /**
             * \brief Filters for \link ArgusEvent events \endlink relating to a
             *        Window.
             *
             * \param event The passed ArgusEvent.
             * \param user_data A pointer to the Window to filter events for.
             *
             * \return Whether the event should be passed along for handling.
             */
            static bool event_filter(ArgusEvent &event, void *user_data);

            /**
             * \brief Handles \link ArgusEvent events \endlink relating to a
             *        Window.
             *
             * \param event The passed ArgusEvent.
             * \param user_data A pointer to the Window to handle events for.
             */
            static void event_callback(ArgusEvent &event, void *user_data);

        public:
            /**
             * \brief Creates a new Window.
             *
             * \return The created Window.
             *
             * \warning Not all platforms may support multiple
             *          \link Window Windows \endlink.
             *
             * \attention The Window is created in heap memory, and will be
             *            deallocated by Window#destroy(void).
             */
            static Window &create_window(void);

            /**
             * \brief Destroys this window.
             *
             * \warning This method destroys the Window object. No other methods
             * should be invoked upon it after calling destroy().
             */
            void destroy(void);

            /**
             * \brief Creates a new window as a child of this one.
             *
             * \return The new child window.
             *
             * \note The child window will not be modal to the parent.
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
            void set_title(const std::string &title);

            /**
             * \brief Sets the fullscreen state of the window.
             *
             * Caution: This may not be supported on all platforms.
             *
             * \param fullscreen Whether the window is to be displayed in
             *        fullscreen.
             */
            void set_fullscreen(const bool fullscreen);

            /**
             * \brief Sets the resolution of the window when not in fullscreen
             *        mode.
             *
             * \param width The new width of the window.
             * \param height The new height of the window.
             *
             * \warning This may not be supported on all platforms.
             */
            void set_resolution(const unsigned int width, const unsigned int height);

            /**
             * \brief Sets the position of the window on the screen when in
             *        windowed mode.
             *
             * \param x The new X-coordinate of the window.
             * \param y The new Y-coordinate of the window.
             *
             * \warning This may not be supported on all platforms.
             */
            void set_windowed_position(const int x, const int y);

            /**
             * \brief Sets the WindowCallback to invoke upon this window being
             *        closed.
             *
             * \param callback The callback to be executed.
             */
            void set_close_callback(WindowCallback callback);

            /**
             * \brief Activates the window.
             *
             * \note This function should be invoked only once.
             */
            void activate(void);
    };

    /**
     * \brief Provides methods for creating new \link Renderable Renderables
     *        \endlink associated with a particular RenderGroup.
     */
    class RenderableFactory {
        friend class RenderGroup;

        private:
            RenderGroup &parent;

            RenderableFactory(RenderGroup &parent);

        public:
            /**
             * \brief Creates a new RenderableTriangle with the given vertices.
             *
             * \param corner_1 The first corner of the triangle.
             * \param corner_2 The second corner of the triangle.
             * \param corner_3 The third corner of the triangle.
             *
             * \return The created RenderableTriangle.
             */
            RenderableTriangle &create_triangle(const Vertex &corner_1, const Vertex &corner_2, const Vertex &corner_3) const;

            /**
             * \brief Creates a new RenderableSquare with the given vertices.
             *
             * \param corner_1 The first corner of the square.
             * \param corner_2 The second corner of the square.
             * \param corner_3 The third corner of the square.
             * \param corner_4 The fourth corner of the square.
             *
             * \return The created RenderableTriangle.
             */
            RenderableSquare &create_square(const Vertex &corner_1, const Vertex &corner_2,
                    const Vertex &corner_3, const Vertex &corner_4) const;
    };

    /**
     * \brief Represents a group of \link Renderable Renderables \endlink to be
     * rendered at once.
     *
     * A RenderGroup may contain both its own Transform and \link Shader Shaders
     * \endlink, which will be applied in conjunction with the respective
     * properties of its parent RenderLayer.
     */
    class RenderGroup {
        friend Renderable;
        friend RenderLayer;
        friend RenderableFactory;

        private:
            /**
             * \brief The RenderLayer which this group belongs to.
             */
            RenderLayer &parent;
            /**
             * \brief The Renderable objects contained by this group.
             */
            std::vector<Renderable*> children;

            /**
             * \brief The Transform of this group.
             *
             * This will be combined with the Transform of the parent
             * RenderLayer.
             */
            Transform transform;

            /**
             * \brief The \link Shader Shaders \endlink to be applied to this
             *        group.
             *
             * These will be combined with the \link Shader Shaders \endlink of
             * the parent RenderLayer.
             */
            std::vector<const Shader*> shaders;

            /**
             * \brief A map of texture IDs to texture array indices.
             */
            std::map<std::string, unsigned int> texture_indices;

            /**
             * \brief The RenderableFactory associated with this group.
             *
             * RenderGroup and RenderableFactory objects always have a
             * one-to-one mapping.
             *
             * \sa RenderableFactory
             */
            RenderableFactory renderable_factory;

            /**
             * \brief The current total vertex count of this group.
             */
            size_t vertex_count;

            /**
             * \brief Whether the child Renderable list has been mutated since
             *        the list was last flushed to the underlying vertex buffer
             *        object.
             */
            bool dirty_children;
            /**
             * \brief Whether the shader list of either this object or its
             *        parent RenderLayer has been mutated since the full shader
             *        list was last compiled.
             */
            bool dirty_shaders;

            bool shaders_initialized;
            bool buffers_initialized;

            ShaderProgram shader_program;
            /**
             * A handle to the underlying vertex buffer object of this group.
             *
             * \attention The exact semantic meaning of this value is
             *            implementation-defined.
             */
            handle_t vbo;
            /**
             * A handle to the underlying vertex array object of this group.
             *
             * \attention The exact semantic meaning of this value is
             *            implementation-defined.
             */
            handle_t vao;
            /**
             * A handle to the underlying texture object of this group.
             *
             * \attention The exact semantic meaning of this value is
             *            implementation-defined.
             */
            handle_t tex_handle;

            /**
             * \brief Constructs a new RenderGroup.
             *
             * \param parent The RenderLayer which will serve as parent to the
             *        new group.
             */
            RenderGroup(RenderLayer &parent);

            /**
             * \brief Constructs the ShaderProgram which will be associated with
             *        this group.
             *
             * \return The constructed ShaderProgram.
             */
            ShaderProgram generate_initial_program(void);

            /**
             * \brief Rebuilds the texture array associated with this group.
             *
             * \param force Whether the textures should be rebuilt regardless of
             *        whether they have been modified.
             */
            void rebuild_textures(bool force);

            /**
             * \brief Updates the vertex buffer and array objects associated
             *        with this group, flushing any changes to the child
             *        Renderable objects.
             */
            void update_buffer(void);

            /**
             * \brief Rebuilds this group's own and inherited
             *        \link Shader Shaders \endlink if needed, updating uniforms
             *        as rqeuired.
             */
            void rebuild_shaders(void);

            /**
             * \brief Draws this group to the screen.
             */
            void draw(void);

            /**
             * \brief Adds a Renderable as a child of this group.
             *
             * \param renderable The Renderable to add as a child.
             */
            void add_renderable(const Renderable &renderable);

            /**
             * \brief Removes a Renderable from this group's children list.
             *
             * \param renderable The Renderable to remove from the children
             *        list.
             *
             * \attention This does not de-allocate the Renderable object, which
             *            must be done separately.
             *
             * \sa Renderable#destroy(void)
             */
            void remove_renderable(const Renderable &renderable);

        public:
            /**
             * \brief Destroys this object.
             *
             * \warning This method destroys the object. No other methods
             *          should be invoked upon it afterward.
             */
            void destroy(void);

            /**
             * \brief Gets the local Transform of this group.
             *
             * \return The local Transform.
             *
             * \remark This Transform is local to the parent RenderLayer, and
             *         does not necessarily reflect the group's transform in
             *         absolute screen space.
             */
            Transform &get_transform(void);

            /**
             * \brief Returns a factory for creating
             *        \link Renderable Renderables \endlink attached to this
             *        RenderGroup.
             *
             * \return This RenderGroup's Renderable factory.
             */
            RenderableFactory &get_renderable_factory(void);

            /**
             * \brief Adds a local Shader to this group.
             *
             * \param shader The Shader to add.
             */
            void add_shader(const Shader &shader);

            /**
             * \brief Removes a local Shader from this group.
             *
             * \param shader The Shader to remove.
             */
            void remove_shader(const Shader &shader);
    };

    /**
     * \brief Represents a layer to which geometry may be rendered.
     *
     * RenderLayers will be composited to the screen as multiple ordered layers
     * when a frame is rendered.
     */
    class RenderLayer {
        friend class Renderer;
        friend class RenderGroup;
        friend class RenderableFactory;

        private:
            /**
             * \brief The Renderer parent to this layer.
             */
            Renderer &parent_renderer;
            /**
             * \brief The priority of this layer.
             *
             * Higher-priority layers will be rendered later, on top of
             * lower-priority ones.
             */
            const int priority;

            /**
             * \brief The \link RenderGroup RenderGroups \endlink contained by
             *        this layer.
             */
            std::vector<RenderGroup*> children;
            /**
             * \brief The \link Shader Shaders \endlink contained by this layer.
             */
            std::vector<const Shader*> shaders;

            /**
             * \brief The implicit root RenderGroup of this layer.
             *
             * \remark A pointer to this RenderGroup is present in the children
             *         vector.
             */
            RenderGroup root_group; // this depends on shaders being initialized

            /**
             * \brief The Transform of this RenderLayer.
             *
             * \sa RenderGroup#get_transform
             */
            Transform transform;

            /**
             * \brief Whether the shader list has been modified since it was
             *        last built.
             */
            bool dirty_shaders;

            /**
             * \brief Constructs a new RenderLayer.
             *
             * \param parent The Renderer parent to the layer.
             * \param priority The priority of the layer.
             *
             * \sa RenderLayer#priority
             */
            RenderLayer(Renderer &parent, const int priority);

            ~RenderLayer(void) = default;

            /**
             * \brief Renders this layer to the screen.
             */
            void render(void);

            /**
             * \brief Removes the given RenderGroup from this layer.
             *
             * \param group The RenderGroup to remove.
             */
            void remove_group(RenderGroup &group);

        public:
            /**
             * \brief Destroys this RenderLayer and removes it from the parent
             *        Renderer.
             */
            void destroy(void);

            /**
             * \brief Gets the Transform of this layer.
             *
             * \return The layer's Transform.
             */
            Transform &get_transform(void);

            /**
             * \brief Returns a factory for creating
             *        \link Renderable Renderables \endlink attached to this
             *        this RenderLayer's root RenderGroup.
             *
             * \returns This RenderLayer's root Renderable factory.
             */
            RenderableFactory &get_renderable_factory(void);

            /**
             * \brief Creates a new RenderGroup as a child of this layer.
             *
             * \param priority The priority of the new RenderGroup.
             *
             * \return RenderGroup The created RenderGroup.
             */
            RenderGroup &create_render_group(const int priority);

            /**
             * \brief Adds the given Shader to this layer.
             *
             * \param shader The Shader to add.
             */
            void add_shader(const Shader &shader);

            /**
             * \brief Removes the given Shader from this layer.
             *
             * \param shader The Shader to remove.
             */
            void remove_shader(const Shader &shader);
    };

    /**
     * \brief Represents an item to be rendered.
     *
     * Each item may have its own rendering properties, as well as a list of
     * child items. Child items will inherit the Transform of their respective
     * parent, which is added to their own.
     */
    class Renderable {
        friend class RenderGroup;

        private:
            /**
             * \brief Releases the handle on the underlying Resource for this
             *        Renderable's Texture.
             */
            void release_texture(void);

            /**
             * \brief The raw vertex buffer data for this Renderable.
             */
            float *vertex_buffer;
            /**
             * \brief The current offset into the vertex buffer.
             *
             * \remark This is used for writing data to the buffer.
             */
            size_t buffer_head;
            /**
             * \brief The current number of elements in the vertex buffer.
             */
            size_t buffer_size;
            /**
             * \brief The current capacity in elements of the vertex buffer.
             */
            size_t max_buffer_size;

            /**
             * \brief The index of this Renderable's texture in the parent
             *        RenderGroup's texture array.
             */
            unsigned int tex_index;
            /**
             * \brief The UV coordinates of this Renderable's texture's
             *        bottom-right corner with respect to the parent
             *        RenderGroup's underlying texture array.
             */
            Vector2f tex_max_uv;
            /**
             * \brief Whether the texture has been modified since being flushed
             *        to the parent RenderGroup.
             */
            std::atomic_bool dirty_texture;

        protected:
            /**
             * \brief The parent RenderGroup of this Renderable.
             */
            RenderGroup &parent;

            /**
             * \brief This Renderable's current Transform.
             */
            Transform transform;

            /**
             * \brief The Resource containing the texture to be applied to this
             *        Renderable.
             *
             * \remark This may be `nullptr` if no Texture is to be applied.
             */
            Resource *tex_resource;

            /**
             * \brief Constructs a new Renderable object.
             *
             * \param parent The RenderGroup parent to the new Renderable.
             */
            Renderable(RenderGroup &parent);

            ~Renderable(void);

            /**
             * \brief Re-allocates the vertex buffer to fit the given number of
             *        vertices.
             *
             * \param vertex_count The number of vertices to allocate for.
             *
             * \remark If the vertex buffer is already large enough to fit the
             *         given vertex count, this function will effectively be a
             *         no-op.
             * \remark The referenced vertex buffer exists in system memory, and
             *         is copied to graphics memory on invocation of
             *         RenderGroup#update_buffer(void).
             *
             * \sa Renderable#buffer_vertex(const Vertex&)
             * \sa Renderable#populate_buffer(void)
             */
            void allocate_buffer(const size_t vertex_count);

            /**
             * \brief Copies a Vertex to the vertex buffer.
             *
             * \param vertex The Vertex to buffer.
             *
             * \sa Renderable#allocate_buffer(const size_t)
             * \sa Renderable#populate_buffer(void)
             */
            void buffer_vertex(const Vertex &vertex);

            /**
             * \brief Populates the vertex buffer with this Renderable's current
             *        vertex data.
             *
             * \sa Renderable#allocate_buffer(const size_t)
             * \sa Renderable#buffer_vertex(const Vertex&)
             */
            virtual void populate_buffer(void) = 0;

            /**
             * \brief Gets the current vertex count of this Renderable.
             *
             * \return The current vertex count of this Renderable.
             */
            virtual const unsigned int get_vertex_count(void) const = 0;

        public:
            /**
             * \brief Removes this Renderable from its parent RenderGroup and
             *        destroys it.
             *
             * \sa RenderGroup#remove_renderable(Renderable&)
             */
            void destroy(void);

            /**
             * \brief Gets the Transform of this Renderable.
             *
             * \return Transform This Renderable's Transform.
             */
            const Transform &get_transform(void) const;

            /**
             * \brief Applies the Texture with the given resource UID to this
             *        Renderable.
             *
             * This method will automatically attempt to load the Resource if
             * necessary.
             *
             * \param texture_uid The UID of the Resource containing the new
             *        texture.
             *
             * \throw ResourceException If the underlying texture Resource
             *        cannot be loaded.
             */
            void set_texture(const std::string &texture_uid);
    };

    /**
     * \brief Represents a simple triangle to be rendered.
     */
    class RenderableTriangle : public Renderable {
        friend class RenderableFactory;

        private:
            const Vertex corner_1;
            const Vertex corner_2;
            const Vertex corner_3;

            /**
             * \brief Constructs a new RenderableTriangle.
             *
             * \param RenderGroup The parent RenderGroup of the new
             *        RenderableTriangle.
             * \param corner_1 The first corner of the new triangle.
             * \param corner_2 The second corner of the new triangle.
             * \param corner_3 The third corner of the new triangle.
             */
            RenderableTriangle(RenderGroup &parent_group, const Vertex &corner_1, const Vertex &corner_2,
                    const Vertex &corner_3);

            void populate_buffer(void) override;

            const unsigned int get_vertex_count(void) const override;
    };

    /**
     * \brief Represents a simple square to be rendered.
     *
     * \remark Squares are actually rendered to the screen as two adjacent
     *         triangles.
     */
    class RenderableSquare : public Renderable {
        friend class RenderableFactory;

        private:
            const Vertex corner_1;
            const Vertex corner_2;
            const Vertex corner_3;
            const Vertex corner_4;

            /**
             * \brief Constructs a new RenderableSquare.
             *
             * \param RenderGroup The parent RenderGroup of the new
             *        RenderableSquare.
             * \param corner_1 The first corner of the new square.
             * \param corner_2 The second corner of the new square.
             * \param corner_3 The third corner of the new square.
             * \param corner_4 The fourth corner of the new square.
             */
            RenderableSquare(RenderGroup &parent_group, const Vertex &corner_1, const Vertex &corner_2,
                    const Vertex &corner_3, const Vertex &corner_4);

            void populate_buffer(void) override;

            const unsigned int get_vertex_count(void) const override;
    };

}
