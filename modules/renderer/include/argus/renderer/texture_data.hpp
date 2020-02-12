/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

#include <atomic>

#include <cstdio>

namespace argus {
    struct pimpl_TextureData;

    /**
     * \brief Contains metadata and data pertaining to an image to be used as a
     *        texture for rendering.
     *
     * Depending on whether the data has been prepared by the renderer, the
     * object may or may not contain the image data. Image data is deleted
     * after it has been uploaded to the GPU during texture preparation.
     */
    struct TextureData {
        friend class RenderGroup;

        private:
            pimpl_TextureData *pimpl;

        public:
            /**
             * \brief The width in pixels of the texture.
             */
            const size_t width;
            /**
             * \brief The height in pixels of the texture.
             */
            const size_t height;

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
        TextureData(const size_t width, const size_t height, unsigned char **&&image_data);

        /**
         * \brief Destroys this object, deleting any buffers in system or video
         *        memory currently in use.
         */
        ~TextureData(void);

        /**
         * \brief Gets whether the texture data has been prepared for use in
         *        rendering.
         *
         * \return Whether the texture data has been prepared.
         */
        const bool is_prepared(void);

        /**
         * \brief Prepares the texture data for use in rendering.
         */
        void prepare(void);
    };
}