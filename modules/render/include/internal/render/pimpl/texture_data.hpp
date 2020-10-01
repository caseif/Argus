/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module render
#include "argus/render/texture_data.hpp"

namespace argus {
    struct pimpl_TextureData {
        /**
         * \brief A two-dimensional array of pixel data for this texture.
         *
         * \remark The data is stored in column-major form.
         */
        unsigned char **image_data;
    };
}
