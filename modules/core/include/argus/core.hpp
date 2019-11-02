/*
 * This file is a part of Argus.
 * Copyright (c) 2019, Max Roncace <mproncace@gmail.com>
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

#define SDL_MAIN_HANDLED

#include <functional>
#include <memory>

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
     * \brief An enumeration of all modules offered by the engine.
     *
     * Each enum value corresponds to a particular bit position, such that the
     * bitwise OR of multiple modules will preserve the set of input modules.
     */
    enum class EngineModules : uint64_t {
        LOWLEVEL    = 0x01,
        CORE        = 0x02,
        INPUT       = 0x04,
        RESMAN      = 0x08,
        RENDERER    = 0x10,
    };

    /**
     * \brief Bitwise OR implementation for EngineModules bitmask elements.
     *
     * \param lhs Left-hand operand.
     * \param rhs Right-hand operand.
     *
     * \return The bitwise OR of the operands.
     */
    EngineModules operator |(const EngineModules lhs, const EngineModules rhs);
    /**
     * \brief Bitwise OR-assignment implementation for EngineModules bitmask
     *        elements.
     *
     * \param lhs Left-hand operand.
     * \param rhs Right-hand operand.
     *
     * \return The bitwise OR of the operands.
     *
     * \sa EngineModules::operator|
     */
    constexpr inline EngineModules operator |=(const EngineModules lhs, const EngineModules rhs);
    /**
     * \brief Bitwise AND implementation for EngineModules bitmask elements.
     *
     * \param lhs Left-hand operand.
     * \param rhs Right-hand operand.
     *
     * \return The bitwise AND of the operands.
     */
    inline bool operator &(const EngineModules lhs, const EngineModules rhs);

    /**
     * \brief Represents the stages of engine bring-up or spin-down.
     */
    enum class LifecycleStage {
        /**
         * \brief The first lifecycle stage.
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
         * being fully de-initializedm as well as for final deallocation and
         * similar tasks.
         */
        POST_DEINIT
    };

    /**
     * \brief Represents a class of event dispatched by the engine.
     */
    enum class ArgusEventType {
        /**
         * \brief An event of an unknown or undefined class.
         */
        UNDEFINED,
        /**
         * \brief An event pertaining to a game window.
         */
        WINDOW,
        /**
         * \brief An event pertaining to keyboard input.
         */
        KEYBOARD,
        /**
         * \brief An event pertaining to mouse input.
         */
        MOUSE,
        /**
         * \brief An event pertaining to joystick input.
         */
        JOYSTICK,
        /**
         * \brief An event signifying some type of abstracted input.
         */
        INPUT
    };

    /**
     * \brief Represents an event pertaining to the current application,
     *        typically triggered by user interaction.
     */
    struct ArgusEvent {
        /**
         * \brief The type of event.
         */
        const ArgusEventType type;
    };

    /**
     * \brief A callback for passing lifecycle changes to engine modules.
     */
    typedef std::function<void(LifecycleStage)> LifecycleUpdateCallback;

    /**
     * \brief An update callback accepts a single parameter representing the
     *        delta in microseconds since the last update.
     */
    typedef std::function<void(TimeDelta)> DeltaCallback;

    /**
     * \brief A callback accepts no parameters.
     */
    typedef std::function<void()> NullaryCallback;

    /**
     * \brief A callback that accepts an event and a piece of user-supplied
     *        data.
     */
    typedef std::function<bool(ArgusEvent&, void*)> ArgusEventFilter;

    /**
     * \brief A callback that accepts an event and a piece of user-supplied
     *        data.
     */
    typedef std::function<void(ArgusEvent&, void*)> ArgusEventCallback;

    /**
     * \brief Initializes the engine with the given modules.
     *
     * \param module_bitmask A bitmask denoting which modules should be
     *        initialized, constructed from the bitwise OR of the appropriate
     *        EngineModule values. MODULE_CORE is implicit.
     *
     * This must be called before any other interaction with the engine takes
     * place.
     */
    void initialize_engine(const EngineModules module_bitmask);

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
     * \param data The data pointer to supply to the filter and callback
     *        functions on each invocation.
     *
     * \return The ID of the new registration.
     */
    const Index register_event_handler(const ArgusEventFilter filter, const ArgusEventCallback callback, void *const data);

    /**
     * \brief Unregisters an event handler.
     *
     * \param id The ID of the callback to unregister.
     */
    void unregister_event_handler(const Index id);

    /**
     * \brief Dispatches an event as wrapped by a unique_ptr.
     *
     * This function is intended for internal use only, and is exposed here
     * solely due to C++ templating restrictions.
     *
     * \param event An rreference to the event to be dispatched as wrapped by a
     *        std::unique_ptr.
     */
    void _dispatch_event_ptr(std::unique_ptr<ArgusEvent> &&event);

    /**
     * Dispatches an event to all respective registered listeners.
     *
     * \param event An lreference to the event to be dispatched.
     */
    template <typename EventType>
    void dispatch_event(const EventType &event) {
        _dispatch_event_ptr(std::make_unique<EventType>(event));
    }

}
