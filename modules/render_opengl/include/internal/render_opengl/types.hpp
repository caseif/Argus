/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2021, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

#include "argus/lowlevel/math.hpp"

#include "aglet/aglet.h"

namespace argus {
    extern Matrix4 g_view_matrix;

    // all typedefs here serve purely to provide semantic information to declarations

    typedef GLuint buffer_handle_t;
    typedef GLuint array_handle_t;
    typedef GLuint texture_handle_t;
    typedef GLuint shader_handle_t;
    typedef GLuint program_handle_t;
    typedef GLint uniform_location_t;
}
