extern crate argus;

use std::process;
use std::sync::atomic::{AtomicBool, Ordering};
use std::time::Duration;
use argus_logging::*;
use clap::Parser;
use argus_core::{init_logger, initialize_engine, load_client_config, start_engine, stop_engine};

crate_logger!(LOGGER, "Bootstrap");

static FORCE_STOP_ON_NEXT_CTRLC: AtomicBool = AtomicBool::new(false);

fn main() {
    ctrlc::set_handler(move || {
        if FORCE_STOP_ON_NEXT_CTRLC.swap(true, Ordering::Relaxed) {
            warn!(LOGGER, "Forcibly stopping engine");
            process::exit(1);
        } else {
            debug!(LOGGER, "Sending engine stop request");
            stop_engine();
        }
    })
        .expect("Failed to set interrupt handler");

    let args = Cli::parse();
    let namespace = args.namespace;

    let mut log_settings = LogSettings::default();
    log_settings.prelude = vec![PreludeComponent::Channel, PreludeComponent::Level];
    init_logger(log_settings);

    info!(LOGGER, "Loading client config...");
    if let Err(err) = load_client_config(namespace) {
        severe!(LOGGER, "Failed to load client config: {}", err);
        flush_log();
        process::exit(1);
    };
    info!(LOGGER, "Loaded client config");

    if let Err(err) = initialize_engine() {
        severe!(LOGGER, "Failed to load initialize engine: {}", err);
        flush_log();
        process::exit(1);
    }
    info!(LOGGER, "Engine initialized");

    info!(LOGGER, "Starting engine...");
    #[allow(irrefutable_let_patterns)]
    if let Err(err) = start_engine() {
        severe!(LOGGER, "Failed to start engine: {}", err);
        flush_log();
        process::exit(1);
    }
}

fn flush_log() {
    LogManager::instance().flush(Duration::from_secs(5)).unwrap();
}

#[derive(Parser)]
#[command(version, about, long_about = None)]
struct Cli {
    #[arg(value_name = "namespace")]
    namespace: String,
}
