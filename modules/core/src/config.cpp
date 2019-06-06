#include "argus/core.hpp"
#include "internal/config.hpp"

namespace argus {
    
    EngineConfig g_engine_config;

    void set_target_framerate(unsigned int target_fps) {
        g_engine_config.target_fps = target_fps;
    }
}
