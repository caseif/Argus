/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2023, Max Roncace <mproncace@protonmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <atomic>

#include <cstdio>

namespace argus {
    // forward declarations
    class Canvas;

    struct pimpl_TextureData;

    /**
     * \brief Contains metadata and data pertaining to an image to be used as a
     *        texture for rendering.
     *
     * Depending on whether the data has been prepared by the renderer, the
     * object may or may not contain the image data. Image data is deleted
     * after it has been uploaded to the GPU during texture preparation.
     */
    class TextureData {
      public:
        pimpl_TextureData *m_pimpl;

        /**
         * \brief The width in pixels of the texture.
         */
        const unsigned int m_width;
        /**
         * \brief The height in pixels of the texture.
         */
        const unsigned int m_height;

        /**
         * \brief Constructs a new instance of this class with the given
         *        metadata and pixel data.
         *
         * \param width The width of the texture in pixels.
         * \param height The height of the texture in pixels.
         * \param image_data A pointer to a column-major 2D-array containing the
         *        texture's pixel data. This *must* point to heap memory. The
         *        calling method's copy of the pointer will be set to nullptr.
         *
         * \attention The pixel data must be in RGBA format with a bit-depth of 8.
         */
        TextureData(const unsigned int width, const unsigned int height, unsigned char **&&image_data);

        TextureData(const TextureData &) noexcept;

        TextureData(TextureData &&) noexcept;

        /**
         * \brief Destroys this object, deleting any buffers in system or video
         *        memory currently in use.
         */
        ~TextureData(void);

        /**
         * \brief Returns a two-dimensional array of pixel data for the
         *        texture.
         *
         * \return The pixel data of the texture;
         */
        const unsigned char *const *&get_pixel_data(void) const;
    };
}
