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

#include "internal/render_opengles/state/viewport_state.hpp"

namespace argus {
    // forward declarations
    struct RendererState;

    ViewportState::ViewportState(RendererState &parent_state, AttachedViewport *viewport) :
            parent_state(parent_state),
            viewport(viewport),
            fb_primary(0),
            fb_secondary(0),
            color_buf_primary(0),
            color_buf_secondary(0) {
    }

    Viewport2DState::Viewport2DState(RendererState &parent_state, AttachedViewport2D *viewport) :
            ViewportState(parent_state, viewport) {
    }
}
