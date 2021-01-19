/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

/**
 * \file argus/core.hpp
 *
 * Contains core engine functionality, primarily for bootstrapping.
 */

#pragma once

// module lowlevel
#include "argus/lowlevel/math.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <functional>
#include <memory>
#include <string>
#include <vector>

#define MODULE_CORE "core"
#define MODULE_WM "wm"
#define MODULE_ECS "ecs"
#define MODULE_INPUT "input"
#define MODULE_RESMAN "resman"
#define MODULE_RENDER "render"

/**
 * \brief Macro for conveniently registering Argus modules contained by shared
 *        libraries.
 *
 * This macro implicitly invokes argus::register_module upon library load.
 *
 * \remark If desired, the library entry point may be specified manually and the
 * argus::register_argus_module function invoked explicitly, removing any need
 * for this macro.
 *
 * \param id The ID of the module.
 * \param layer The layer of the module.
 * \param dependencies A list of IDs of modules this one is dependent on.
 * \param lifecycle_update_callback The function which handles lifecycle updates
 *        for this module.
 *
 * \sa argus::ArgusModule
 * \sa argus::register_argus_module
 */
#ifdef _WIN32
#define REGISTER_ARGUS_MODULE(id, layer, dependencies, lifecycle_update_callback) \
    BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) { \
        argus::register_module(argus::ArgusModule{id, layer, dependencies, lifecycle_update_callback}); \
        return true; \
    }
#elif defined(__GNUC__) || defined(__clang__)
#define REGISTER_ARGUS_MODULE(id, layer, dependencies, lifecycle_update_callback) \
    __attribute__((constructor)) void __argus_module_ctor(void) { \
        argus::register_module(argus::ArgusModule{id, layer, dependencies, lifecycle_update_callback}); \
    }
#else
#error This platform is not supported.
#endif

namespace argus {

    /**
     * \brief Represents an instant in time.
     */
    typedef unsigned long long Timestamp;

    /**
     * \brief Represents a duration of time.
     */
    typedef unsigned long long TimeDelta;

    /**
     * \brief Represents a unique index used for tracking purposes.
     */
    typedef unsigned long long Index;

    /**
     * \brief Represents the stages of engine bring-up or spin-down.
     */
    enum class LifecycleStage {
        /**
         * \brief The first standard lifecycle stage.
         *
         * Should be used for performing early allocation or other early setup,
         * generally for the purpose of preparing the module for use in the
         * initialization of dependent modules.
         */
        PRE_INIT,
        /**
         * \brief Primary initialization stage, for performing most
         *        initialization tasks.
         */
        INIT,
        /**
         * \brief Post-initialization stage, for performing initialization
         *        contingent on all parent modules being initialized.
         */
        POST_INIT,
        /**
         * \brief Early de-initialization. This occurs immediately after the
         *        engine has committed to shutting down.
         *
         * Should be used for performing early de-initialization tasks, such as
         * saving user data. Changes during this stage should not be visible to
         * dependent modules.
         */
        PRE_DEINIT,
        /**
         * \brief Primary de-initialization.
         *
         * Should be used for performing most de-initialization tasks.
         */
        DEINIT,
        /**
         * \brief Very late initialization.
         *
         * Should be used for performing de-init contingent on parent modules
         * being fully de-initialized as well as for final deallocation and
         * similar tasks.
         */
        POST_DEINIT
    };

    /**
     * \brief Represents a class of event dispatched by the engine.
     */
    enum class ArgusEventType : uint16_t {
        /**
         * \brief An event of an unknown or undefined class.
         */
        UNDEFINED = 0x01,
        /**
         * \brief An event pertaining to a game window.
         */
        WINDOW = 0x02,
        /**
         * \brief An event pertaining to keyboard input.
         */
        KEYBOARD = 0x04,
        /**
         * \brief An event pertaining to mouse input.
         */
        MOUSE = 0x08,
        /**
         * \brief An event pertaining to joystick input.
         */
        JOYSTICK = 0x10,
        /**
         * \brief An event signifying some type of abstracted input.
         */
        INPUT = KEYBOARD | MOUSE | JOYSTICK
    };

    /**
     * \brief Bitwise OR implementation for ArgusEventType bitmask elements.
     *
     * \param lhs Left-hand operand.
     * \param rhs Right-hand operand.
     *
     * \return The bitwise OR of the operands.
     */
    constexpr ArgusEventType operator|(const ArgusEventType lhs, const ArgusEventType rhs);
    /**
     * \brief Bitwise OR-assignment implementation for ArgusEventType bitmask
     *        elements.
     *
     * \param lhs Left-hand operand.
     * \param rhs Right-hand operand.
     *
     * \return The bitwise OR of the operands.
     *
     * \sa KeyboardModifiers::operator|
     */
    constexpr inline ArgusEventType operator|=(const ArgusEventType lhs, const ArgusEventType rhs);
    /**
     * \brief Bitwise AND implementation for KeyboardModifiers bitmask elements.
     *
     * \param lhs Left-hand operand.
     * \param rhs Right-hand operand.
     *
     * \return The bitwise AND of the operands.
     */
    constexpr inline ArgusEventType operator&(const ArgusEventType lhs, const ArgusEventType rhs);

    /**
     * \brief Represents an event pertaining to the current application,
     *        typically triggered by user interaction.
     */
    struct ArgusEvent {
       protected:
        /**
         * \brief Aggregate constructor for ArgusEvent.
         *
         * \param type The \link ArgusEventType type \endlink of event.
         */
        ArgusEvent(ArgusEventType type);

       public:
        /**
         * \brief The type of event.
         */
        const ArgusEventType type;
    };

    enum class TargetThread {
        UPDATE,
        RENDER
    };

    /**
     * \brief Represents a graphics backend used to instantiate a Window and
     *        corresponding Renderer.
     *
     * \warning A Vulkan-based renderer is not yet implemented.
     */
    enum class RenderBackend {
        OPENGL = 0x01,
        OPENGLES = 0x02,
        VULKAN = 0x11
    };

    /**
     * \brief A callback for passing lifecycle changes to engine modules.
     */
    typedef std::function<void(const LifecycleStage)> LifecycleUpdateCallback;

    /**
     * \brief An update callback accepts a single parameter representing the
     *        delta in microseconds since the last update.
     */
    typedef std::function<void(const TimeDelta)> DeltaCallback;

    /**
     * \brief A callback accepts no parameters.
     */
    typedef std::function<void()> NullaryCallback;

    /**
     * \brief A callback that accepts an event and a piece of user-supplied
     *        data.
     */
    typedef std::function<void(const ArgusEvent &, void *)> ArgusEventCallback;

    /**
     * \brief Represents a module for the Argus engine.
     *
     * This struct contains all information required to initialize and update
     * the module appropriately.
     */
    struct ArgusModule {
        /**
         * \brief The ID of the module.
         *
         * \attention This ID must contain only lowercase Latin letters
         *            (`[a-z]`), numbers (`[0-9]`), and underscores (`[_]`).
         */
        const std::string id;
        /**
         * \brief The layer of the module.
         *
         * A module may be dependent on another module only if the dependency
         * specifies a lower layer than the dependent. For example, a module
         * on with layer `3` may depend on a module on layer `2`, but not on one
         * on `3` or `4`. This requirements removes any possibility of circular
         * dependencies by necessitating a well-defined load order.
         */
        const uint8_t layer;
        /**
         * \brief A list of IDs of modules this one is dependent on.
         *
         * If any dependency fails to load, the dependent module will also fail.
         */
        const std::vector<std::string> dependencies;
        /**
         * \brief The function which handles lifecycle updates for this module.
         *
         * This function will accept a single argument of type `const`
         * LifecycleStage and will not return anything.
         *
         * This function should handle initialization of the module when the
         * engine starts, as well as deinitialization when the engine stops.
         *
         * \sa LifecycleStage
         */
        const LifecycleUpdateCallback lifecycle_update_callback;
    };

    /**
     * \brief Registers a module for use with the engine.
     *
     * This function should be invoked upon the module library being loaded.
     *
     * \attention For convenience, the macro REGISTER_ARGUS_MODULE registers an
     *            entry point which invokes this function automatically.
     *
     * \throw std::invalid_argument If a module with the given ID is already
     *        registered.
     */
    void register_module(const ArgusModule module);

    /**
     * \brief Enables a registered module on demand.
     *
     * \param module_id The ID of the module to enable.
     *
     * \throw std::invalid_argument If no module with the given ID is currently
     *        registered.
     */
    void enable_module(const std::string module_id);

    /**
     * \brief Initializes the engine.
     *
     * argus::set_load_modules(const std::initializer_list) should be invoked
     * before this function is called. If the load modules have not been
     * configured, only the `core` module will be loaded.
     *
     * \attention This must be called before any other interaction with the
     * engine takes place.
     *
     * \throw std::invalid_argument If any of the requested modules (or their
     *        dependencies) cannot be loaded.
     */
    void initialize_engine(void);

    /**
     * \brief Starts the engine.
     *
     * \param game_loop The callback representing the main game loop.
     */
    void start_engine(const DeltaCallback game_loop);

    /**
     * \brief Requests that the engine halt execution, performing cleanup as
     *        necessary.
     */
    void stop_engine(void);

    /**
     * \brief Sets the target tickrate of the engine.
     *
     * When performance allows, the engine will sleep between updates to
     * enforce this limit. Set to 0 to disable tickrate targeting.
     *
     * \param target_tickrate The new target tickrate in updates/second.
     *
     * \attention This is independent from the target framerate, which controls
     *            how frequently frames are rendered.
     */
    void set_target_tickrate(const unsigned int target_tickrate);

    /**
     * \brief Sets the target framerate of the engine.
     *
     * When performance allows, the engine will sleep between frames to
     * enforce this limit. Set to 0 to disable framerate targeting.
     *
     * \param target_framerate The new target framerate in frames/second.
     *
     * \attention This is independent from the target tickrate, which controls
     *            how frequently the game logic routine is called.
     */
    void set_target_framerate(const unsigned int target_framerate);

    /**
     * \brief Sets the modules to load on engine initialization.
     *
     * If any provided module or any its respective dependencies cannot be
     * loaded, engine initialization will fail.
     *
     * \param module_list The IDs of the modules to load on engine init.
     */
    void set_load_modules(const std::initializer_list<std::string> &module_list);

    /**
     * \brief Returns a list of graphics backends available for use on the
     *        current platform.
     *
     * \return The available graphics backends.
     */
    std::vector<RenderBackend> get_available_render_backends(void);

    /**
     * \brief Sets the graphics backend to be used for rendering.
     *
     * \param backend A list of render backends to use in order of preference.
     *
     * \remark This option is treated like a "hint" and will not be honored in
     *          the event that the preferred backend is not available, either
     *          due to a missing implementation or lack of hardware support. If
     *          none of the specified backends can be used, the OpenGL backend
     *          will be used as the default fallback.
     */
    void set_render_backend(const std::initializer_list<RenderBackend> backend);

    /**
     * \brief Sets the graphics backend to be used for rendering.
     *
     * \param backend The preferred RenderBackend to use.
     *
     * \remark This option is treated like a "hint" and will not be honored in
     *          the event that the preferred backend is not available, either
     *          due to a missing implementation or lack of hardware support. If
     *          none of the specified backends can be used, the OpenGL backend
     *          will be used as the default fallback.
     */
    void set_render_backend(const RenderBackend backend);

    /**
     * \brief Sets the screen space used to compute the projection matrix
     *        passed to shader programs.
     *
     * If this value is not provided, it will default to [-1, 1] on both axes.
     *
     * \param screen_space The parameters for the screen space.
     */
    void set_screen_space(ScreenSpace screen_space);

    /**
     * \brief Sets the screen space used to compute the projection matrix
     *        passed to shader programs.
     *
     * If this value is not provided, it will default to [-1, 1] on both axes.
     *
     * \param left The x-coordinate corresponding to the left side of the
     *        screen.
     * \param right The x-coordinate corresponding to the right side of the
     *        screen.
     * \param bottom The y-coordinate corresponding to the bottom side of the
     *        screen.
     * \param top The y-coordinate corresponding to the top side of the
     *        screen.
     */
    void set_screen_space(float left, float right, float bottom, float top);

    /**
     * \brief Registers a callback for invocation on each game update.
     *
     * It is normally not necessary to invoke this from game code.
     *
     * \param update_callback The callback to be invoked on each update.
     *
     * \return The ID of the new registration.
     *
     * \sa DeltaCallback
     */
    const Index register_update_callback(const DeltaCallback update_callback);

    /**
     * \brief Unregisters an update callback.
     *
     * \param id The ID of the callback to unregister.
     */
    void unregister_update_callback(const Index id);

    /**
     * \brief Registers a callback for invocation on each render update.
     *
     * It is normally not necessary to invoke this from game code.
     *
     * \param render_callback The callback to be invoked on each frame.
     *
     * \return The ID of the new registration.
     *
     * \sa DeltaCallback
     */
    const Index register_render_callback(const DeltaCallback render_callback);

    /**
     * \brief Unregisters a render callback.
     *
     * \param id The ID of the callback to unregister.
     */
    void unregister_render_callback(const Index id);

    /**
     * \brief Registers a handler for particular events.
     *
     * Events which match the given filter will be passed to the callback
     * function along with the user-supplied data pointer.
     *
     * \param filter The \link ArgusEventFilter filter \endlink for the new
     *        event handler.
     * \param callback The \link ArgusEventCallback callback \endlink
     *        responsible for handling passed events.
     * \param target_thread The thread to invoke the handler function on.
     * \param data The data pointer to supply to the filter and callback
     *        functions on each invocation.
     *
     * \return The ID of the new registration.
     */
    const Index register_event_handler(const ArgusEventType type, const ArgusEventCallback callback,
            const TargetThread target_thread, void *const data = nullptr);

    /**
     * \brief Unregisters an event handler.
     *
     * \param id The ID of the callback to unregister.
     */
    void unregister_event_handler(const Index id);

    /**
     * \brief Dispatches an event.
     *
     * This function is intended for internal use only, and is exposed here
     * solely due to C++ templating restrictions.
     *
     * \param event The event to be dispatched.
     */
    void _dispatch_event_ptr(const ArgusEvent &event, size_t obj_size);

    /**
     * \brief Dispatches an event to all respective registered listeners.
     *
     * \param event An lreference to the event to be dispatched.
     */
    template <typename EventType>
    void dispatch_event(const EventType &event) {
        _dispatch_event_ptr(event, sizeof(event));
    }

}
