use crate::rerun::run_if_changed;
use arp::{create_arp_from_fs, CompressionType, PackingOptions};
use std::path::PathBuf;
use std::{env, fs};

const RES_MAPPINGS_PATH: &str = "../../../res/arp_custom_mappings.csv";

const RES_SOURCES_PATH: &str = "res/";

const GENERATED_OUT_PREFIX: &str = "generated/";

const ARP_OUT_PREFIX: &str = "arp/";

pub fn pack_builtin_resources() {
    let crate_root = env::current_dir().expect("Failed to get current directory");
    let res_srcs_path = crate_root.join(RES_SOURCES_PATH);
    if !res_srcs_path.exists() {
        panic!("Module does not appear to have any resource files under ./res");
    }

    run_if_changed(&res_srcs_path, &|| {
        let out_dir = PathBuf::from(env::var("OUT_DIR").unwrap());
        let gen_dir = out_dir.join(GENERATED_OUT_PREFIX);
        if !gen_dir.exists() {
            fs::create_dir(&gen_dir).expect("Failed to created directory for generated sources");
        }
        let pack_out_dir = gen_dir.join(ARP_OUT_PREFIX);

        let out_name = "resources";

        let opts = PackingOptions::new_v1(
            out_name,
            "argus",
            None, // max part size
            Some(CompressionType::Deflate), // compression
            Some(RES_MAPPINGS_PATH),
        )
            .expect("Failed to build ARP packing options");
        create_arp_from_fs(RES_SOURCES_PATH, pack_out_dir, opts).expect("Failed to build ARP package");
    });
}
