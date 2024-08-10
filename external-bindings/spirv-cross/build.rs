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
    let header_path: PathBuf = [
        manifest_dir.as_str(),
        "..",
        "..",
        "external",
        "libs",
        "SPIRV-Cross",
        "spirv_cross_c.h",
    ].into_iter().collect();

    let bindings = bindgen::builder()
        .header(header_path.to_str().unwrap())
        .prepend_enum_name(false)
        .merge_extern_blocks(true)
        .clang_arg("-std=c11")
        .clang_arg("-DSPIRV_CROSS_C_API_HLSL=0")
        .clang_arg("-DSPIRV_CROSS_C_API_MSL=0")
        .clang_arg("-DSPIRV_CROSS_C_API_REFLECT=0")
        .generate()
        .expect("Failed to generate bindings");

    bindings.write_to_file(bindings_path).expect("Failed to write bindings");
}
