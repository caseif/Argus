use std::{env, fs};
use std::fs::OpenOptions;
use std::path::{Path, PathBuf};

const FNV_OFFSET_BASIS: u64 = 0xcbf29ce484222325;
const FNV_PRIME: u64 = 0x100000001b3;

pub fn run_if_changed(path: &Path, f: &dyn Fn()) {
    let timestamp_file = get_timestamp_file(path);
    if !timestamp_file.exists() || did_file_change(path, &timestamp_file) {
        f();
        touch_timestamp_file(path);
    }
    println!("cargo::rerun-if-changed={}", path.to_str().unwrap());
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

fn did_file_change(path: &Path, timestamp_file: &Path) -> bool {
    if !path.exists() {
        panic!(
            "No file exists at path {}",
            path.to_string_lossy()
        );
    }

    if path.is_dir() {
        for entry in fs::read_dir(path).expect("Failed to read directory") {
            let entry = entry.expect("Failed to read directory entry");
            if did_file_change(&entry.path(), timestamp_file) {
                return true;
            }
        }
        false
    } else {
        let cur_modified_time = match fs::metadata(path).and_then(|meta| meta.modified()) {
            Ok(modified_time) => modified_time,
            Err(e) => panic!(
                "Failed to read metadata of file at path {} ({e})",
                path.to_str().unwrap(),
            ),
        };

        let prev_modified_time = match fs::metadata(timestamp_file).and_then(|meta| meta.modified()) {
            Ok(modified_time) => modified_time,
            Err(e) => panic!(
                "Failed to read metadata of timestamp file at path {} ({e})",
                timestamp_file.to_str().unwrap(),
            ),
        };

        cur_modified_time >= prev_modified_time
    }
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

    if timestamp_file.exists() {
        fs::remove_file(&timestamp_file).expect("Failed to remove timestamp file");
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
