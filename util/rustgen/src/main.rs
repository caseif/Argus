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

use std::collections::{HashMap, HashSet, VecDeque};
use std::path::PathBuf;

#[allow(dead_code)]
#[derive(Copy, Clone, Eq, PartialEq, Hash)]
enum ModuleType {
    Library,
    Static,
    Dynamic,
    Auxiliary,
    Executable,
}

#[derive(Clone, Eq, PartialEq, Hash)]
struct ModuleDef {
    mod_type: ModuleType,
    name: String,
}

impl ModuleDef {
    fn new(mod_type: &ModuleType, name: &str) -> ModuleDef {
        return ModuleDef { mod_type: mod_type.to_owned(), name: name.to_owned() };
    }
}

type DepGraph = HashMap<ModuleDef, Vec<ModuleDef>>;

const MODULE_TYPES: [ModuleType; 5] = [
    ModuleType::Library,
    ModuleType::Static,
    ModuleType::Dynamic,
    ModuleType::Auxiliary,
    ModuleType::Executable
];

fn get_module_type_dir(mod_type: &ModuleType) -> &'static str {
    match mod_type {
        ModuleType::Library => "libs",
        ModuleType::Static => "static",
        ModuleType::Dynamic => "dynamic",
        ModuleType::Auxiliary => "auxiliary",
        ModuleType::Executable => "exe",
    }
}

const PROP_LIB_DEPS: &str = "engine_library_deps";
const PROP_STATIC_DEPS: &str = "engine_module_deps";
const PROP_AUX_DEPS: &str = "engine_aux_deps";

const MODULES: [(ModuleType, &'static str); 3] = [
    (ModuleType::Library, "lowlevel"),
    (ModuleType::Static, "core"),
    (ModuleType::Static, "resman"),
];

fn get_bindings_path(module_name: &str) -> PathBuf {
    return ["..", "..", "engine", "auxiliary", format!("{module_name}_rustabi").as_str(), "src",
        format!("{module_name}_cabi").as_str(), "bindings.rs"]
            .iter().collect();
}

fn get_module_include_dir(module_type: &ModuleType, module_name: &str) -> PathBuf {
    return ["..", "..", "engine", get_module_type_dir(module_type), module_name, "include"].iter().collect();
}

fn get_main_header_path(module_type: &ModuleType, module_name: &str) -> PathBuf {
    return get_module_include_dir(module_type, module_name)
            .join(&["argus", format!("{module_name}_cabi.h").as_str()].iter().collect::<PathBuf>());
}

fn build_dependency_graph() -> DepGraph {
    let mut edges = DepGraph::new();

    for mod_type in &MODULE_TYPES {
        let base_path: PathBuf = ["..", "..", "engine", get_module_type_dir(mod_type)].iter().collect();
        for path in std::fs::read_dir(&base_path).unwrap().into_iter().flat_map(|opt| opt) {
            let mut mod_props_path = path.path();
            mod_props_path.push("module.properties");
            if !mod_props_path.is_file() {
                continue;
            }

            let src_def = ModuleDef::new(mod_type, path.file_name().as_os_str().to_str().unwrap());

            let props = std::fs::read_to_string(mod_props_path).unwrap().split("\n").into_iter()
                    .filter(|s| s.contains("="))
                    .map(|s| s.split_once("=").unwrap())
                    .map(|(k, v)| (k.trim().to_string(), v.trim().to_string()))
                    .collect::<HashMap<String, String>>();

            if props.contains_key(PROP_LIB_DEPS) {
                for dep in props[PROP_LIB_DEPS].split(",").map(|s| s.trim()) {
                    let dst_def = ModuleDef::new(&ModuleType::Library, dep);
                    edges.entry(src_def.to_owned())
                            .and_modify(|v| v.push(dst_def.clone()))
                            .or_insert(vec![dst_def.clone()]);
                }
            }

            if props.contains_key(PROP_STATIC_DEPS) {
                for dep in props[PROP_STATIC_DEPS].split(",").map(|s| s.trim()) {
                    let dst_def = ModuleDef::new(&ModuleType::Static, dep);
                    edges.entry(src_def.to_owned())
                            .and_modify(|v| v.push(dst_def.clone()))
                            .or_insert(vec![dst_def.clone()]);
                }
            }

            if props.contains_key(PROP_AUX_DEPS) {
                for dep in props[PROP_AUX_DEPS].split(",").map(|s| s.trim()) {
                    let dst_def = ModuleDef::new(&ModuleType::Auxiliary, dep);
                    edges.entry(src_def.to_owned())
                            .and_modify(|v| v.push(dst_def.clone()))
                            .or_insert(vec![dst_def.clone()]);
                }
            }
        }
    }

    return edges;
}

fn get_all_dependencies(module: &ModuleDef, graph: &DepGraph) -> HashSet<ModuleDef> {
    let mut set = HashSet::<ModuleDef>::new();
    let mut queue = VecDeque::<&ModuleDef>::new();
    queue.push_back(module);

    while let Some(cur_mod) = queue.pop_front() {
        set.insert(cur_mod.clone());

        if !graph.contains_key(&cur_mod) {
            continue;
        }

        for dep in &graph[&cur_mod] {
            if !set.contains(&dep) && !queue.contains(&dep) {
                queue.push_back(dep);
            }
        }
    }

    return set;
}

fn gen_bindings((module_type, module_name): &(ModuleType, &'static str), dep_graph: &DepGraph) {
    let bindings_path = get_bindings_path(module_name);
    let header_path = get_main_header_path(&module_type, module_name).canonicalize()
            .expect("Main header does not exist");

    let mut module_inc_dirs = Vec::<PathBuf>::new();
    for dep in get_all_dependencies(&ModuleDef::new(&module_type, module_name), dep_graph) {
        module_inc_dirs.push(get_module_include_dir(&dep.mod_type, &dep.name).canonicalize()
                .expect("Module include directory does not exist"));
    }

    let crate_src_dir = bindings_path.parent().unwrap();

    if !crate_src_dir.is_dir() {
        panic!("Cannot generate bindings: Crate source directory does not exist or is not a directory");
    }

    if !header_path.is_file() {
        panic!("Cannot generate bindings for module: Main header does not exist or is not a file");
    }

    let bindings = bindgen::builder()
            .header(header_path.to_str().unwrap())
            .prepend_enum_name(false)
            .enable_function_attribute_detection()
            .merge_extern_blocks(true)
            .layout_tests(false)
            .allowlist_file(format!("^.*[/\\\\]argus[/\\\\]{}[/\\\\].*$", module_name))
            .allowlist_file(format!("^.*[/\\\\]argus[/\\\\]{}\\.h(pp)?$", module_name))
            .allowlist_file(format!("^.*[/\\\\]glslang[/\\\\](.+[/\\\\])*.+\\.h(pp)?$"))
            .allowlist_recursively(false)
            .clang_args(
                module_inc_dirs.iter()
                    .map(|s| format!("-I{}", s.to_str().unwrap()))
                    .collect::<Vec<String>>()
            )
            .clang_arg("-std=c11")
            .raw_line("#![allow(non_camel_case_types, unused_imports, unused_qualifications)]")
            .raw_line("use super::*;")
            .generate()
            .expect("Failed to generate bindings");

    bindings.write_to_file(bindings_path).expect("Failed to write bindings to file");
}

fn main() {
    let dep_graph = build_dependency_graph();

    for module in &MODULES {
        gen_bindings(module, &dep_graph);
    }
}
