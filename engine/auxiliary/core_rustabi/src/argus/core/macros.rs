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

#![allow(dead_code)]

use crate::core_cabi;

const ENGINE_NAME: &str = unsafe { std::str::from_utf8_unchecked(core_cabi::ARGUS_ENGINE_NAME) };
const ENGINE_VERISON_MAJOR: u32 = core_cabi::ARGUS_ENGINE_VERSION_MAJOR;
const ENGINE_VERISON_MINOR: u32 = core_cabi::ARGUS_ENGINE_VERSION_MINOR;
const ENGINE_VERISON_INCR: u32 = core_cabi::ARGUS_ENGINE_VERSION_INCR;
