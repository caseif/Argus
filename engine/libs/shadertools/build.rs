use std::{env, path::Path};

const PROJECT_NAME: &'static str = "shadertools";

fn main() {
    let module_dir_str = env::var("CARGO_MANIFEST_DIR").unwrap();
    let module_dir = Path::new(&module_dir_str);
    let root_dir = module_dir.parent().unwrap().parent().unwrap().parent().unwrap();
    let libs_dir = root_dir.join("build").join("external_projects").join(PROJECT_NAME).join("libs");

    println!("cargo:rustc-link-search=native={}", libs_dir.display());
    
    println!("cargo:rustc-link-lib=static={}", "glslang");
    println!("cargo:rustc-link-lib=static={}", "SPIRV");
}
