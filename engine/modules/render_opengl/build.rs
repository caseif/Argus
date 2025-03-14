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

use std::path::{Path, PathBuf};
use std::process::Command;
use std::{env, fs};
use argus_build_util::rerun::run_if_changed;
use argus_build_util::resource::pack_builtin_resources;

const RUBY_EXE: &str = "ruby";
#[cfg(windows)]
const BUNDLER_EXES: &[&str] = &["bundle.bat", "bundle.cmd"];
#[cfg(not(windows))]
const BUNDLER_EXES: &[&str] = &["bundle"];

const AGLET_SUBMODULE_PATH: &str = "../../../external/tooling/aglet/";
const GL_PROFILE_PATH: &str = "tooling/aglet/opengl_profile.xml";

const GENERATED_OUT_PREFIX: &str = "generated/";

const BINDINGS_OUT_PREFIX: &str = "aglet/";

fn main() {
    let crate_root = env::current_dir().expect("Failed to get current directory");
    let gl_profile_path = crate_root.join(GL_PROFILE_PATH);
    run_if_changed(&gl_profile_path, &|| {
        generate_opengl_bindings(&gl_profile_path)
    });

    pack_builtin_resources();
}

fn generate_opengl_bindings(profile_path: &Path) {
    let out_dir = PathBuf::from(env::var("OUT_DIR").unwrap());
    let gen_dir = out_dir.join(GENERATED_OUT_PREFIX);
    if !gen_dir.exists() {
        fs::create_dir(&gen_dir).expect("Failed to created directory for generated sources");
    }
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
                    String::from_utf8(res.stderr).unwrap(),
                );
            }
            let res_2 = Command::new(bundler_exe)
                .current_dir(working_dir)
                .output()
                .expect("Bundler executable disappeared!");
            if !res_2.status.success() {
                panic!(
                    "Bundler exited with code {} ({})",
                    res_2.status,
                    String::from_utf8(res_2.stderr).unwrap(),
                );
            }

            return;
        }
    }

    // if we get here then none of the exe names worked
    panic!("Bundler must be installed and available on the path (run `$ gem install bundler`)");
}
