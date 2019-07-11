#pragma once

#include <functional>

namespace argus {

    typedef enum class engine_modules_t : uint64_t {
        CORE        = 0x01,
        LOWLEVEL    = 0x02,
        RENDERER    = 0x04,
    } EngineModules;

    EngineModules operator |(const EngineModules lhs, const EngineModules rhs);
    
    bool operator &(const EngineModules lhs, const EngineModules rhs);

    /**
     * \brief An update callback accepts a single parameter representing the
     *        delta in microseconds since the last update.
     */
    typedef std::function<void(unsigned long long)> DeltaCallback;

    /**
     * \brief A callback accepts no parameters.
     */
    typedef std::function<void()> NullaryCallback;

    /*
     * \brief Initializes the engine with the given modules.
     *
     * \param module_bitmask A bitmask denoting which modules should be
     * initialized, constructed from the bitwise OR of the appropriate
     * EngineModule values. MODULE_CORE is implicit.
     * 
     * This must be called before any other interaction with the engine takes
     * place.
     */
    void initialize_engine(EngineModules module_bitmask);

    /**
     * \brief Starts the engine.
     *
     * \param game_loop The callback representing the main game loop.
     */
    void start_engine(DeltaCallback game_loop);

    /**
     * \brief Requests that the engine halt execution, performing cleanup as
     * necessary.
     */
    void stop_engine(void);

    /**
     * \brief Sets the target tickrate of the engine.
     *
     * When performance allows, the engine will sleep between updates to
     * maintain this limit. Set to 0 to disable tickrate targeting.
     */
    void set_target_framerate(unsigned int target_tickrate);

    /**
     * \brief Registers a callback for invocation on each game update.
     *
     * It is normally not necessary to invoke this from game code.
     */
    void register_update_callback(DeltaCallback update_callback);

    /**
     * \brief Registers a callback for invocation on each render update.
     *
     * It is normally not necessary to invoke this from game code.
     */
    void register_render_callback(DeltaCallback update_callback);

    /**
     * \brief Registers a callback for invocation when the engine is requested
     * to close.
     *
     * It is normally not necessary to invoke this from game code.
     */
    void register_close_callback(NullaryCallback close_callback);

}
