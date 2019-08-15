#pragma once

// module core
#include "argus/core.hpp"

#define VMMLIB_OLD_TYPEDEFS
#include "vmmlib/matrix.hpp"
#include "vmmlib/vector.hpp"

#include <functional>
#include <initializer_list>
#include <string>
#include <unordered_map>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_render.h>

namespace argus {

    using vmml::vec2f;
    using vmml::vec2d;
    using vmml::mat4f;

    // forward declarations
    class Transform;
    class Shader;
    class ShaderProgram;
    class Renderer;
    class RenderLayer;
    class RenderGroup;
    class Renderable;
    class RenderableFactory;
    class RenderableTriangle;

    class Transform {
        private:
            Transform *parent;
            vec2d translation;
            double rotation;
            vec2d scale;

            bool dirty;

        public:
            Transform(void);

            Transform(vec2d const &translation, const double rotation, vec2d const &scale);

            ~Transform(void);

            Transform operator +(const Transform rhs);

            vec2d const &get_translation(void) const;

            void set_translation(vec2d const &translation);

            void add_translation(vec2d const &translation_delta);
            
            const double get_rotation(void) const;

            void set_rotation(const double rotation_degrees);

            void add_rotation(const double rotation_degrees);

            vec2d const &get_scale(void) const;

            void set_scale(vec2d const &scale);

            void set_parent(Transform &transform);

            mat4f const &to_matrix(void) const;

            const bool is_dirty(void) const;

            void clean(void);
    };

    class Shader {
        friend class ShaderProgram;

        private:
            const GLenum type;
            const std::string src;
            const std::string entry_point;
            const std::initializer_list<std::string> uniform_ids;

            Shader(const GLenum type, std::string const &src, std::string const &entry_point,
                    std::initializer_list<std::string> uniform_ids);

        public:
            static Shader &create_vertex_shader(const std::string src, const std::string entry_point,
                    const std::initializer_list<std::string> uniform_ids);

            static Shader create_vertex_shader_stack(const std::string src, const std::string entry_point,
                    const std::initializer_list<std::string> uniform_ids);

            static Shader &create_fragment_shader(const std::string src, const std::string entry_point,
                    const std::initializer_list<std::string> uniform_ids);

            static Shader create_fragment_shader_stack(const std::string src, const std::string entry_point,
                    const std::initializer_list<std::string> uniform_ids);
    };

    class ShaderProgram {
        private:
            std::vector<const Shader*> shaders;
            std::vector<GLint> shader_handles;
            std::unordered_map<std::string, GLint> uniforms;

            bool built;

            GLuint gl_program;

            void link(void);

        public:
            ShaderProgram(const std::vector<const Shader*> &shaders);

            void delete_program(void);

            void use_program(void);

            GLuint get_handle(void) const;

            GLint get_uniform_location(std::string const &uniform_id) const;
    };

    class Window {
        friend class Renderer;

        private:
            SDL_Window *handle;

            uint64_t callback_id;
            uint64_t listener_id;

            Window *parent;
            std::vector<Window*> children;

            Renderer &renderer;

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

    class Renderer {
        friend class Window;

        private:
            Window &window;
            SDL_GLContext gl_context;

            std::vector<RenderLayer*> render_layers;

            Renderer(Window &window);

            ~Renderer(void);

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

    /**
     * Represents a layer to which geometry may be rendered.
     *
     * Render layers will be composited to the screen as multiple ordered
     * layers when a frame is rendered.
     */
    class RenderLayer {
        friend class Renderer;
        friend RenderGroup;
        friend class RenderableFactory;

        private:
            Renderer *parent_renderer;

            std::vector<RenderGroup*> children;
            RenderGroup &root_group;

            Transform transform;

            std::vector<const Shader*> shaders;

            bool dirty_transform;
            bool dirty_shaders;

            GLuint framebuffer;
            GLuint gl_texture;

            RenderLayer(Renderer *const parent);

            ~RenderLayer(void) = default;

            void render(void);
        
        public:
            /**
             * \brief Destroys this RenderLayer and removes it from the parent
             * renderer.
             */
            void destroy(void);

            Transform &get_transform(void);

            RenderGroup &create_render_group(const int priority);

            void add_shader(Shader const &shader);

            void remove_shader(Shader const &shader);
    };

    class RenderGroup {
        friend RenderLayer;
        friend Renderable;

        private:
            RenderLayer &parent;
            std::vector<Renderable*> children;

            Transform transform;

            std::vector<const Shader*> shaders;

            RenderableFactory &renderable_factory;

            size_t vertex_count;

            bool dirty_transform;
            bool dirty_children;
            bool dirty_shaders;

            ShaderProgram shader_program;
            GLuint vbo;
            GLuint vao;
            
            RenderGroup(RenderLayer &parent);

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
             * this RenderLayer.
             *
             * \returns This RenderLayer's Renderable factory.
             */
            RenderableFactory &get_renderable_factory(void) const;

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

            virtual void render(const GLuint vbo, const size_t offset) const = 0;

            virtual const unsigned int get_vertex_count(void) const = 0;

        public:
            void remove(void);

            Transform const &get_transform(void) const;
    };

    class RenderableTriangle : Renderable {
        friend class RenderableFactory;

        private:
            const vec2f corner_1;
            const vec2f corner_2;
            const vec2f corner_3;

            RenderableTriangle(RenderGroup &parent_group, vec2f const &corner_1, vec2f const &corner_2, vec2f const &corner_3);

            void render(const GLuint vbo, const size_t offset) const override;

            const unsigned int get_vertex_count(void) const override;
    };

    class RenderableFactory {
        friend class RenderGroup;

        private:
            RenderGroup &parent;

            RenderableFactory(RenderGroup &parent);

        public:
            RenderableTriangle &create_triangle(vec2d const &corner_1, vec2d const &corner_2, vec2d const &corner_3) const;
    };

}
