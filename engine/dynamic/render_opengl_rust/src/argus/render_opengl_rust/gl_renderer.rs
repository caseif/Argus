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

use std::time::Duration;

use crate::argus::render_opengl_rust::state::{RendererState};
use lowlevel_rustabi::argus::lowlevel::Vector2u;
use wm_rustabi::argus::wm::{Window};

pub(crate) struct GlRenderer {
    window: Window,
    state: RendererState,
}

impl GlRenderer {
    pub(crate) fn new(window: Window) -> Self {
        Self {
            window,
            state: Default::default(),
        }
    }

    pub(crate) fn render(&mut self, delta: Duration) {
        //TODO
    }

    pub(crate) fn notify_window_resize(&mut self, new_size: Vector2u) {
        //TODO
    }
}
