// module core
#include "argus/core.hpp"
#include "internal/config.hpp"

namespace argus {

    EngineConfig g_engine_config;

    void set_target_tickrate(const unsigned int target_tickrate) {
        g_engine_config.target_tickrate = target_tickrate;
    }

    void set_target_framerate(const unsigned int target_framerate) {
        g_engine_config.target_framerate = target_framerate;
    }

}
