#include "argus/core.hpp"
#include "argus/lowlevel.hpp"
#include "internal/config.hpp"

#include <chrono>
#include <iostream>
#include <vector>

#define US_PER_S 1000000LLU
#define SLEEP_OVERHEAD_NS 120000LLU

namespace argus {

    static std::vector<Callback> g_update_callbacks;
    static std::vector<Callback> g_render_callbacks;

    extern EngineConfig g_engine_config;

    bool initialized = false;

    unsigned long long g_last_update = 0;

    static void _handle_idle(unsigned long long frame_start) {
        if (g_engine_config.target_fps != 0) {
            unsigned long long delta = argus::microtime() - frame_start;

            unsigned int frametime_target_us = US_PER_S / g_engine_config.target_fps;
            if (delta < frametime_target_us) {
                unsigned long long sleep_time_ns = (frametime_target_us - delta) * 1000;
                if (sleep_time_ns <= SLEEP_OVERHEAD_NS) {
                    return;
                }
                sleep_nanos(sleep_time_ns - SLEEP_OVERHEAD_NS);
            }
        }
    }

    static void _game_loop(void) {
        while (1) {
            unsigned long long frame_start = argus::microtime();
            unsigned long long last_delta;

            if (g_last_update != 0) {
                last_delta = argus::microtime() - g_last_update;
            } else {
                last_delta = 0;
            }
            g_last_update = argus::microtime();

            // invoke update callbacks
            std::vector<Callback>::const_iterator it;
            for (it = g_update_callbacks.begin(); it != g_update_callbacks.end(); it++) {
                (*it)(last_delta);
            }

            _handle_idle(frame_start);
        }

        return;
    }

    void initialize_engine(void) {
        ASSERT(initialized, "Cannot initialize engine more than once.");

        // we'll probably register around 10 or so internal callbacks, so allocate them now
        g_update_callbacks.reserve(10);

        //TODO: more init stuff

        initialized = true;
        return;
    }

    void register_update_callback(Callback callback) {
        ASSERT(initialized, "Cannot register update callback before engine initialization.");

        g_update_callbacks.insert(g_update_callbacks.cend(), callback);
    }

    void register_render_callback(Callback callback) {
        g_render_callbacks.insert(g_render_callbacks.cend(), callback);
    }

    void start_engine(Callback game_loop) {
        ASSERT(initialized, "Cannot start engine before it is initialized.");
        ASSERT(game_loop != NULL, "start_engine invoked with null callback");

        register_update_callback(game_loop);

        // pass control over to the game loop
        _game_loop();

        exit(0);
    }

}
