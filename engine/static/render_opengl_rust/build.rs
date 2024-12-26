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

use std::fs::OpenOptions;
use std::path::{Path, PathBuf};
use std::process::Command;
use std::{env, fs};

const FNV_OFFSET_BASIS: u64 = 0xcbf29ce484222325;
const FNV_PRIME: u64 = 0x100000001b3;

const RUBY_EXE: &str = "ruby";
#[cfg(windows)]
const BUNDLER_EXES: &[&str] = &["bundle.bat", "bundle.cmd"];
#[cfg(not(windows))]
const BUNDLER_EXES: &[&str] = &["bundle"];

const ARPTOOL_EXE: &str = "arptool";
const ARPTOOL_PATH_ENV_VAR: &str = "ARPTOOL_PATH";

const AGLET_SUBMODULE_PATH: &str = "../../../external/tooling/aglet/";
const GL_PROFILE_PATH: &str = "tooling/aglet/opengl_profile.xml";

const RES_SOURCES_PATH: &str = "res/";
const RES_MAPPINGS_PATH: &str = "../../../res/arp_custom_mappings.csv";

const GENERATED_OUT_PREFIX: &str = "generated/";

const BINDINGS_OUT_PREFIX: &str = "aglet/";

const ARP_OUT_PREFIX: &str = "arp/";

fn main() {
    let crate_root = env::current_dir().expect("Failed to get current directory");
    let gl_profile_path = crate_root.join(GL_PROFILE_PATH);
    run_if_changed(&gl_profile_path, &|| {
        generate_opengl_bindings(&gl_profile_path)
    });

    let res_srcs_path = crate_root.join(RES_SOURCES_PATH);
    run_if_changed(&res_srcs_path, &|| {
        generate_resource_pack(&res_srcs_path);
    });
}

fn generate_opengl_bindings(profile_path: &Path) {
    let out_dir = PathBuf::from(env::var("OUT_DIR").unwrap());
    let gen_dir = out_dir.join(GENERATED_OUT_PREFIX);
    let bindings_out_dir = gen_dir.join(BINDINGS_OUT_PREFIX);

    // check if Ruby is installed
    match Command::new(RUBY_EXE).arg("-v").output() {
        Ok(_) => {}
        Err(_) => panic!("Ruby must be installed and available on the path")
    }

    run_bundler(AGLET_SUBMODULE_PATH);

    let output = Command::new(RUBY_EXE)
        .arg(format!("{AGLET_SUBMODULE_PATH}/aglet.rb"))
        .arg("--lang=rust")
        .args(["-p", profile_path.to_str().unwrap()])
        .args(["-o", bindings_out_dir.to_str().unwrap()])
        .current_dir(AGLET_SUBMODULE_PATH)
        .output()
        .expect("Ruby executable was present but isn't anymore??");
    if !output.status.success() {
        panic!(
            "Aglet exited with code {}\nstderr output:\n    {}",
            output.status,
            String::from_utf8(output.stderr).unwrap(),
        );
    }
}

fn run_bundler(working_dir: &str) {
    for bundler_exe in BUNDLER_EXES {
        if let Ok(res) = Command::new(bundler_exe)
            .args(["config", "set", "--local", "path", "./vendor/cache"])
            .current_dir(working_dir)
            .output() {
            if !res.status.success() {
                panic!(
                    "Bundler exited with code {} ({})",
                    res.status,
                    String::from_utf8(res.stderr).unwrap()
                );
            }
            return;
        }
    }

    // if we get here then none of the exe names worked
    panic!("Bundler must be installed and available on the path (run `$ gem install bundler`)");
}

fn generate_resource_pack(srcs_path: &Path) {
    let out_dir = PathBuf::from(env::var("OUT_DIR").unwrap());
    let gen_dir = out_dir.join(GENERATED_OUT_PREFIX);
    let pack_out_dir = gen_dir.join(ARP_OUT_PREFIX);
    let out_name = "resources";

    let arptool_cmd = env::var(ARPTOOL_PATH_ENV_VAR).unwrap_or(ARPTOOL_EXE.to_string());

    // check if arptool is available on path
    match Command::new(&arptool_cmd).arg("-v").output() {
        Ok(_) => {}
        Err(_) => panic!("ARPTOOL_PATH must be set or arptool must be available on the path")
    }

    let output = Command::new(&arptool_cmd)
        .arg("pack")
        .arg(srcs_path.to_str().unwrap())
        .arg("--namespace=argus")
        .arg(format!("--output={}", pack_out_dir.to_str().unwrap()))
        .arg(format!("--name={out_name}"))
        .arg("--compression=deflate")
        .arg(format!("--mappings={RES_MAPPINGS_PATH}"))
        .arg("--quiet")
        .current_dir(AGLET_SUBMODULE_PATH)
        .output()
        .expect("arptool executable was present but isn't anymore??");
    if !output.status.success() {
        panic!(
            "arptool exited with code {}\nstderr output:\n    {}",
            output.status,
            String::from_utf8(output.stderr).unwrap(),
        );
    }
}

fn fnv1a_hash(s: &str) -> u64 {
    let mut hash = FNV_OFFSET_BASIS;
    for byte in s.as_bytes() {
        hash ^= u64::from(*byte);
        hash = hash.wrapping_mul(FNV_PRIME);
    }
    hash
}

fn get_timestamp_file(subject_path: &Path) -> PathBuf {
    PathBuf::from(env::var("OUT_DIR").unwrap())
        .join("build_timestamps")
        .join(fnv1a_hash(subject_path.to_str().unwrap()).to_string())
}

fn did_file_change(path: &Path) -> bool {
    if !path.exists() {
        panic!(
            "No file exists at path {}",
            path.to_string_lossy().to_string()
        );
    }

    let timestamp_file = get_timestamp_file(path);
    if !timestamp_file.exists() {
        return true;
    }

    let cur_modified_time = match fs::metadata(path).and_then(|meta| meta.modified()) {
        Ok(modified_time) => modified_time,
        Err(e) => panic!(
            "Failed to read metadata of file at path {} ({e})",
            path.to_str().unwrap(),
        ),
    };

    let prev_modified_time = match fs::metadata(&timestamp_file).and_then(|meta| meta.modified()) {
        Ok(modified_time) => modified_time,
        Err(e) => panic!(
            "Failed to read metadata of timestamp file at path {} ({e})",
            timestamp_file.to_str().unwrap(),
        ),
    };

    cur_modified_time.ge(&prev_modified_time)
}

fn touch_timestamp_file(subject_path: &Path) {
    let timestamp_file = get_timestamp_file(subject_path);
    let timestamp_dir = timestamp_file
        .parent()
        .expect("Timestamp file cannot be directly under the root directory");
    if !timestamp_file.exists() {
        match fs::create_dir_all(timestamp_dir) {
            Ok(_) => {}
            Err(e) => panic!(
                "Failed to create directory at {} ({e})",
                timestamp_file.to_str().unwrap(),
            ),
        }
    }

    match OpenOptions::new()
        .create(true)
        .truncate(true)
        .write(true)
        .open(&timestamp_file)
    {
        Ok(_) => {}
        Err(e) => panic!(
            "Failed to touch timestamp file at {} ({e})",
            timestamp_file.to_str().unwrap(),
        ),
    }
}

fn run_if_changed(path: &Path, f: &dyn Fn()) {
    if did_file_change(path) {
        f();
        touch_timestamp_file(path);
    }
    println!("cargo::rerun-if-changed={}", path.to_str().unwrap());
}
