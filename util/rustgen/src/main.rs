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

use std::path::PathBuf;

#[allow(dead_code)]
enum ModuleType {
    Library,
    Static,
    Dynamic,
    Auxiliary,
    Executable,
}

const MODULES: [(ModuleType, &'static str); 1] = [
    (ModuleType::Static, "core"),
];

fn get_module_type_dir(module_type: &ModuleType) -> &'static str {
    match module_type {
        ModuleType::Library => "lib",
        ModuleType::Static => "static",
        ModuleType::Dynamic => "dynamic",
        ModuleType::Auxiliary => "aux",
        ModuleType::Executable => "exe",
    }
}

fn get_bindings_path(module_name: &str) -> PathBuf {
    return ["..", "..", "engine", "aux", format!("{module_name}_rustabi").as_str(), "src", "bindings.rs"]
            .iter().collect();
}

fn get_module_include_dir(module_type: &ModuleType, module_name: &str) -> PathBuf {
    return ["..", "..", "engine", get_module_type_dir(module_type), module_name, "include"].iter().collect();
}

fn get_main_header_path(module_type: &ModuleType, module_name: &str) -> PathBuf {
    return get_module_include_dir(module_type, module_name)
            .join(&["argus", format!("{module_name}_cabi.h").as_str()].iter().collect::<PathBuf>());
}

fn gen_bindings((module_type, module_name): (ModuleType, &'static str)) {
    let bindings_path = get_bindings_path(module_name).canonicalize().unwrap();
    let header_path = get_main_header_path(&module_type, module_name).canonicalize().unwrap();
    let module_inc_dir = get_module_include_dir(&module_type, module_name).canonicalize().unwrap();

    let crate_src_dir = bindings_path.parent().unwrap();

    if !crate_src_dir.is_dir() {
        panic!("Cannot generate bindings: Crate source directory does not exist or is not directory");
    }

    if !header_path.is_file() {
        panic!("Cannot generate bindings for module: Main header does not exist");
    }

    let bindings = bindgen::builder()
            .header(header_path.to_str().unwrap())
            .prepend_enum_name(false)
            .enable_function_attribute_detection()
            .merge_extern_blocks(true)
            .allowlist_function(".*")
            .allowlist_item("ARGUS_.*")
            .clang_arg(format!("-I{}", module_inc_dir.to_str().unwrap()))
            .clang_arg("-std=c11")
            .generate()
            .expect("Failed to generate bindings");

    bindings.write_to_file(bindings_path).expect("Failed to write bindings to file");
}

fn main() {
    for module in MODULES {
        gen_bindings(module);
    }
}
