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

extern crate argus_core;
extern crate argus_game2d;
extern crate argus_input;
extern crate argus_render;
extern crate argus_resman;
extern crate argus_scripting;
extern crate argus_scripting_lua;
extern crate argus_sound;
extern crate argus_ui;
extern crate argus_wm;

#[cfg(feature = "opengl")]
extern crate argus_render_opengl;

#[cfg(feature = "vulkan")]
extern crate argus_render_vulkan;
