/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2020, Max Roncace <mproncace@gmail.com>
 *
 * This software is made available under the MIT license. You should have
 * received a copy of the full license text with this software. If not, the
 * license text may be accessed at https://opensource.org/licenses/MIT.
 */

#pragma once

// module lowlevel
#include "argus/lowlevel/math.hpp"

#include <initializer_list>
#include <string>
#include <vector>

namespace argus {
    /**
     * \brief Sets the target tickrate of the engine.
     *
     * When performance allows, the engine will sleep between updates to
     * enforce this limit. Set to 0 to disable tickrate targeting.
     *
     * \param target_tickrate The new target tickrate in updates/second.
     *
     * \attention This is independent from the target framerate, which controls
     *            how frequently frames are rendered.
     */
    void set_target_tickrate(const unsigned int target_tickrate);

    /**
     * \brief Sets the target framerate of the engine.
     *
     * When performance allows, the engine will sleep between frames to
     * enforce this limit. Set to 0 to disable framerate targeting.
     *
     * \param target_framerate The new target framerate in frames/second.
     *
     * \attention This is independent from the target tickrate, which controls
     *            how frequently the game logic routine is called.
     */
    void set_target_framerate(const unsigned int target_framerate);

    /**
     * \brief Sets the modules to load on engine initialization.
     *
     * If any provided module or any its respective dependencies cannot be
     * loaded, engine initialization will fail.
     *
     * \param module_list The IDs of the modules to load on engine init.
     */
    void set_load_modules(const std::initializer_list<std::string> &module_list);

    /**
     * \brief Represents a graphics backend used to instantiate a Window and
     *        corresponding Renderer.
     *
     * \warning A Vulkan-based renderer is not yet implemented.
     */
    enum class RenderBackend {
        OPENGL = 0x01,
        OPENGLES = 0x02,
        VULKAN = 0x11
    };

    /**
     * \brief Returns a list of graphics backends available for use on the
     *        current platform.
     *
     * \return The available graphics backends.
     */
    std::vector<RenderBackend> get_available_render_backends(void);

    /**
     * \brief Sets the graphics backend to be used for rendering.
     *
     * \param backend A list of render backends to use in order of preference.
     *
     * \remark This option is treated like a "hint" and will not be honored in
     *          the event that the preferred backend is not available, either
     *          due to a missing implementation or lack of hardware support. If
     *          none of the specified backends can be used, the OpenGL backend
     *          will be used as the default fallback.
     */
    void set_render_backend(const std::initializer_list<RenderBackend> backend);

    /**
     * \brief Sets the graphics backend to be used for rendering.
     *
     * \param backend The preferred RenderBackend to use.
     *
     * \remark This option is treated like a "hint" and will not be honored in
     *          the event that the preferred backend is not available, either
     *          due to a missing implementation or lack of hardware support. If
     *          none of the specified backends can be used, the OpenGL backend
     *          will be used as the default fallback.
     */
    void set_render_backend(const RenderBackend backend);

    /**
     * \brief Sets the screen space used to compute the projection matrix
     *        passed to shader programs.
     *
     * If this value is not provided, it will default to [-1, 1] on both axes.
     *
     * \param screen_space The parameters for the screen space.
     */
    void set_screen_space(ScreenSpace screen_space);

    /**
     * \brief Sets the screen space used to compute the projection matrix
     *        passed to shader programs.
     *
     * If this value is not provided, it will default to [-1, 1] on both axes.
     *
     * \param left The x-coordinate corresponding to the left side of the
     *        screen.
     * \param right The x-coordinate corresponding to the right side of the
     *        screen.
     * \param bottom The y-coordinate corresponding to the bottom side of the
     *        screen.
     * \param top The y-coordinate corresponding to the top side of the
     *        screen.
     */
    void set_screen_space(float left, float right, float bottom, float top);
}
