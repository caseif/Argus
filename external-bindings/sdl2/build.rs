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

use std::env;
use std::path::PathBuf;

pub fn main() {
    let out_dir = PathBuf::from(env::var("OUT_DIR").unwrap());
    let bindings_path = out_dir.join("bindings.rs");

    let manifest_dir = env::var("CARGO_MANIFEST_DIR").unwrap();
    let sdl_header_path: PathBuf = [
        manifest_dir.as_str(),
        "..",
        "..",
        "external",
        "libs",
        "sdl2",
        "include",
        "SDL.h",
    ].into_iter().collect();

    let bindings = bindgen::builder()
        .header(sdl_header_path.to_string_lossy().to_string())
        .allowlist_type("wchar_t")
        .blocklist_type("__va_list_tag")
        .blocklist_function("SDL_.+printf")
        .blocklist_function("SDL_.+scanf")
        .blocklist_function("SDL_LogMessageV")
        .prepend_enum_name(false)
        .merge_extern_blocks(true)
        .layout_tests(false)
        .allowlist_file("^.*[/\\\\]SDL_.+\\.h?$")
        .allowlist_file("^.*[/\\\\](begin|end)_code\\.h$")
        .allowlist_recursively(false)
        .clang_arg("-std=c11")
        .generate()
        .expect("Failed to generate bindings");

    bindings.write_to_file(bindings_path).expect("Failed to write bindings");
}
