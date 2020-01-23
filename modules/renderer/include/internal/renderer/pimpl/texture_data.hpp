/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module renderer
#include "argus/renderer/texture_data.hpp"
#include "internal/renderer/types.hpp"

#include <atomic>

namespace argus {
    struct pimpl_TextureData {
        /**
         * \brief A two-dimensional array of pixel data for this texture.
         *
         * The data is stored in column-major form. If the texture has
         * already been prepared for use in rendering, the data will no
         * longer be present in system memory and the pointer will be
         * equivalent to nullptr.
         */
        unsigned char **image_data;

        /**
         * \brief Whether the texture data has been prepared for use.
         */
        std::atomic_bool prepared;
        /**
         * \brief A handle to the buffer in video memory storing this
         *        texture's data. This handle is only valid after the
         *        texture data has been prepared for use.
         */
        handle_t buffer_handle;
    };
}