#pragma once

namespace argus {

    /**
     * \brief An update callback accepts a single parameter representing the
     *        delta in microseconds since the last update.
     */
    typedef void (*UpdateCallback)(unsigned long long);

    /*
     * \brief Performs basic engine initialization.
     * 
     * This must be called before any other interaction with the engine takes
     * place.
     */
    void initialize_engine(void);

    /**
     * \brief Starts the engine.
     *
     * \param update_callback The callback representing the main game loop.
     */
    void start_engine(UpdateCallback update_callback);

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
    void register_update_callback(UpdateCallback update_callback);

}
