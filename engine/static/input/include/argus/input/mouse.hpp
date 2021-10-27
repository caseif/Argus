#pragma once

#include "argus/lowlevel/math.hpp"

namespace argus {
    /**
     * \brief Returns the change in position of the mouse from the previous
     *        frame.
     */
    Vector2d get_mouse_delta(void);
}
