use std::{env, path::Path};
fn main() {
    let module_dir_str = env::var("CARGO_MANIFEST_DIR").unwrap();
    let module_dir = Path::new(&module_dir_str);
    let root_dir = module_dir.parent().unwrap().parent().unwrap().parent().unwrap();
    let libs_dir = root_dir.join("build").join("external").join("libs");

    let glslang_dir = libs_dir.join("glslang").join("glslang");
    let spirv_dir = libs_dir.join("glslang").join("SPIRV");

    println!("cargo:rustc-link-search=native={}", glslang_dir.display());
    println!("cargo:rustc-link-search=native={}", spirv_dir.display());
    println!("cargo:rustc-link-lib=static={}", "glslang");
    println!("cargo:rustc-link-lib=static={}", "SPIRV");
}
