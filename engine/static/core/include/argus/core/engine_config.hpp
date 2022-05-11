/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2022, Max Roncace <mproncace@protonmail.com>
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
     * \brief Sets the modules to load on engine initialization.
     *
     * If any provided module or any its respective dependencies cannot be
     * loaded, engine initialization will fail.
     *
     * \param module_list The IDs of the modules to load on engine init.
     */
    void set_load_modules(const std::vector<std::string> &module_list);

    /**
     * \brief Returns a list of graphics backends available for use on the
     *        current platform.
     *
     * \return The available graphics backends.
     */
    const std::vector<std::string> &get_available_render_backends(void);

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
    void set_render_backends(const std::initializer_list<std::string> &backend);

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
    void set_render_backends(const std::vector<std::string> &backend);

    /**
     * \brief Sets the graphics backend to be used for rendering.
     *
     * \param backend The preferred backend to use.
     *
     * \remark This option is treated like a "hint" and will not be honored in
     *          the event that the preferred backend is not available, either
     *          due to a missing implementation or lack of hardware support. If
     *          none of the specified backends can be used, the OpenGL backend
     *          will be used as the default fallback.
     */
    void set_render_backend(const std::string backend);

    /**
     * \brief Sets the screen space scale mode used to compute the view matrix
     *        passed to shader programs.
     *
     * If this value is not provided, it will default
     * ScreenSpaceScaleMode::NormalizeMinDimension.
     *
     * \param scale_mode The screen space scale mode to use.
     *
     * \sa ScreenSpaceScaleMode
     */
    void set_screen_space_scale_mode(ScreenSpaceScaleMode scale_mode);
}
