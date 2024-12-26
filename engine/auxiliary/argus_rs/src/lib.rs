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

pub use game2d_rs::update_lifecycle_game2d_rs;
pub use render_rs::update_lifecycle_render_rs;
pub use scripting_rs::update_lifecycle_scripting_rs;
pub use shadertools::c_wrapper::*;

#[cfg(feature = "opengl")]
pub use render_opengl_rust::update_lifecycle_render_opengl_rust;
