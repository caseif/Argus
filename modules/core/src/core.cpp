#include "argus/argus_core.hpp"
#include "argus/argus_lowlevel.hpp"

#include <iostream>
#ifdef USE_PTHREADS
#include <pthread.h>
#else
#include <thread>
#endif
#include <vector>

namespace argus {

    static argus::Thread *g_game_thread;

    static std::vector<UpdateCallback> g_update_callbacks(10);

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

        // invoke update callbacks
        std::vector<UpdateCallback>::const_iterator it;
        for (it = g_update_callbacks.begin(); it != g_update_callbacks.end(); it++) {
            (*it)(delta);
        }

        return NULL;
    }

    void start_engine(UpdateCallback update_callback) {
        if (update_callback == NULL) {
            std::cout << "start_engine invoked with NULL update callback" << std::endl;
            exit(1);
        }

        add_update_callback(update_callback);

        // initialize the engine before proceeding
        _init_engine();

        // start the game loop in a new thread
        g_game_thread = argus::thread_create(_game_loop, NULL);

        // return control of the main thread back to the program
        return;
    }

    void add_update_callback(UpdateCallback callback) {
        g_update_callbacks.insert(g_update_callbacks.cend(), callback);
    }

}
