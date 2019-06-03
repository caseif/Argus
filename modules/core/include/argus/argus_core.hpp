#pragma once

namespace argus {

    /* An update callback accepts a single parameter representing the delta   */
    /* in microseconds since the last update.                                 */
    typedef void (*UpdateCallback)(unsigned long long);

    /* Starts the engine using the given function as the game loop callback.  */
    void start_engine(UpdateCallback update_callback);

    /* Adds a callback to be invoked on each game update. It is normally not  */
    /* necessary to invoke this from game code.                               */
    void add_update_callback(UpdateCallback update_callback);

}
