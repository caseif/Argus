/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2024, Max Roncace <mproncace@protonmail.com>
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

#include "argus/render/2d/attached_viewport_2d.hpp"
#include "internal/render_opengl_legacy/types.hpp"

namespace argus {
    // forward declarations
    struct RendererState;

    struct ViewportState {
        RendererState &parent_state;
        AttachedViewport *viewport;

        Matrix4 view_matrix;

        buffer_handle_t fb_primary = 0;
        buffer_handle_t fb_secondary = 0;

        texture_handle_t color_buf_primary = 0;
        texture_handle_t color_buf_secondary = 0;
        // alias of either primary or secondary color buf depending on how many
        // ping-pongs took place
        texture_handle_t color_buf_front = 0;

        texture_handle_t light_opac_map_buf = 0;

        buffer_handle_t lightmap_fb = 0;
        texture_handle_t lightmap_tex = 0;

        ViewportState(RendererState &parent_state, AttachedViewport *viewport);
    };

    struct Viewport2DState : ViewportState {
        Viewport2DState(RendererState &parent_state, AttachedViewport2D *viewport);
    };
}
