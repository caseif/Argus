#include "argus/renderer.hpp"

namespace argus {

    void RenderNull::render(void) const {
        render_children();
        // do nothing else
    }

}
