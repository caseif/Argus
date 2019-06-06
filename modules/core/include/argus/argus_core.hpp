#pragma once

namespace argus {

    /* An update callback accepts a single parameter representing the delta   */
    /* in microseconds since the last update.                                 */
    typedef void (*UpdateCallback)(unsigned long long);

    /* Performs basic engine initialization. This must be called before any   */
    /* other interaction with the engine takes place.                         */
    void initialize_engine(void);

    /* Starts the engine using the given function as the game loop callback.  */
    void start_engine(UpdateCallback update_callback);

    /* Sets the target FPS of the engine. When performance allows, the engine */
    /* will sleep between frames to maintain this limit. Set to 0 to disable  */
    /* framerate targeting.                                                   */
    void set_target_framerate(unsigned int target_fps);

    /* Registers a callback for invocation on each game update. It is         */
    /* normally not necessary to invoke this from game code.                  */
    void register_update_callback(UpdateCallback update_callback);

}
