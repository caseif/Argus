/*
 * This file is a part of Argus.
 * Copyright (c) 2019, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

namespace argus {
    /**
     * \brief An unsigned integer handle to an object.
     */
    typedef unsigned int handle_t;
    /**
     * \brief A signed integer handle to an object.
     */
    typedef int shandle_t;

    /**
     * \brief A handle to a window.
     */
    typedef void *window_handle_t;
    /**
     * \brief A handle to a graphics context.
     */
    typedef void *graphics_context_t;
}
