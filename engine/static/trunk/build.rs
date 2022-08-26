extern crate cargo_metadata;

use std::collections::HashMap;
use std::{env, fs};
use std::path::Path;
use cargo_metadata::{Metadata, MetadataCommand};
use lazy_static::lazy_static;
use regex::Regex;

const META_KEY_MODULE_TYPE: &str = "module-type";

const MODULE_TYPE_LIB: &str = "lib";
const MODULE_TYPE_STATIC: &str = "static";
const MODULE_TYPE_DYNAMIC: &str = "dynamic";

fn main() {
    generate_module_defs();
}

fn generate_module_defs() {
    let path = env::var("CARGO_MANIFEST_DIR").unwrap();
    let crate_meta = MetadataCommand::new()
        .current_dir(&path)
        .manifest_path("../../../Cargo.toml")
        .exec()
        .unwrap();

    let mut lib_mods = HashMap::<String, Metadata>::new();
    let mut static_mods = HashMap::<String, Metadata>::new();
    let mut dynamic_mods = HashMap::<String, Metadata>::new();

    let root = crate_meta.root_package().unwrap();
    for dep in &root.dependencies {
        if let Some(dep_path) = &dep.path {
            let dep_meta = MetadataCommand::new()
                .current_dir(dep_path)
                .manifest_path("./Cargo.toml")
                .exec()
                .unwrap();

            match dep_meta.root_package().unwrap().metadata[META_KEY_MODULE_TYPE].as_str().unwrap_or_default() {
                MODULE_TYPE_LIB => &mut lib_mods,
                MODULE_TYPE_STATIC => &mut static_mods,
                MODULE_TYPE_DYNAMIC => &mut dynamic_mods,
                _ => { continue; }
            }.insert(dep.name.clone(), dep_meta);
        }
    }

    let mut module_defs = HashMap::<String, Vec<String>>::new();

    let name_regex = Regex::new(r"^argus-").unwrap();

    for (mod_name, mod_meta) in &static_mods {
        module_defs.insert(name_regex.replace(mod_name.as_str(), "").as_ref().to_string(),
                           mod_meta.root_package().unwrap().dependencies
            .iter()
            .filter(|d| d.path.is_some())
            .filter(|d| static_mods.contains_key(&d.name))
            .map(|d| name_regex.replace(&d.name, "").to_string())
            .collect());
    }

    /*let update_fns_src = module_defs.iter()
        .map(|(mod_name, _)| format!("fn update_lifecycle_{}(stage: crate::module::LifecycleStage);", mod_name))
        .reduce(|a, b| format!("{}\n{}", a, b))
        .unwrap_or_default();*/
    let update_fns_src = "";

    let defs_src = "lazy_static::lazy_static! {\n    \
        pub static ref STATIC_MODULE_DEFS: std::collections::HashMap<&'static str, Vec<&'static str>> \
            = std::collections::HashMap::from([\n".to_string()
        + module_defs.iter()
            .map(|(mod_name, mod_deps)| format!("        (\"{}\", vec!({})),",
                mod_name,
                mod_deps
                    .iter()
                    .map(|d| format!("\"{}\"", d))
                    .reduce(|a, b| format!("{}, {}", a, b))
                    .unwrap_or_default()
            ))
            .reduce(|a, b| format!("{}\n{}", a, b))
            .unwrap_or_default()
            .as_str()
        + "\n    ]);\n}";

    let combined_src = format!("{}\n\n{}\n", update_fns_src, defs_src);

    let out_dir = env::var_os("OUT_DIR").unwrap();
    let dest_path = Path::new(&out_dir).join("module_defs.rs");

    match fs::write(&dest_path, combined_src) {
        Err(_) => panic!("Failed to generate module_defs.rs"),
        _ => {}
    }

    println!("cargo:rerun-if-changed=build.rs");
}