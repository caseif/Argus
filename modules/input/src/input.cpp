// module input
#include "argus/input.hpp"
#include "internal/input_helpers.hpp"

// module core
#include "argus/core.hpp"

namespace argus {
    
    void update_lifecycle_input(LifecycleStage stage) {
        if (stage == LifecycleStage::INIT) {
            init_keyboard();
        }
    }

}
