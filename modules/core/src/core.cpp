#include "argus/argus_core.hpp"
#include "argus/argus_lowlevel.hpp"

#include <iostream>
#ifdef USE_PTHREADS
#include <pthread.h>
#else
#include <thread>
#endif
#include <time.h>

namespace argus {

    static argus::Thread *g_game_thread;
    static UpdateCallback g_update_callback;

    unsigned long long g_last_update = 0;

    static void _init_engine(void) {
        //TODO
        return;
    }

    static void *_game_loop(void*) {
        unsigned long long delta;
        if (g_last_update != 0) {
            delta = argus::microtime() - g_last_update;
        } else {
            delta = 0;
        }

        // invoke the game's callback
        g_update_callback(delta);

        return NULL;
    }

    void start_engine(UpdateCallback update_callback) {
        if (update_callback == NULL) {
            std::cout << "start_engine invoked with NULL update callback" << std::endl;
            exit(1);
        }

        g_update_callback = update_callback;

        // initialize the engine before proceeding
        _init_engine();

        // start the game loop in a new thread
        g_game_thread = argus::thread_create(_game_loop, NULL);

        // return control of the main thread back to the program
        return;
    }

}
