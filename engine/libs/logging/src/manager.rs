use std::collections::{BTreeMap, HashMap};
use std::io::{stderr, stdout, Write};
use std::sync::atomic::{AtomicU64, Ordering};
use std::sync::mpsc::{channel, Receiver, Sender};
use std::sync::OnceLock;
use std::thread;
use std::thread::JoinHandle;
use std::time::{Instant, SystemTime};
use chrono::{DateTime, Local};
use crate::{LogLevel, LogSettings, PreludeComponent};
use crate::settings::validate_settings;

static MANAGER: OnceLock<LogManager> = OnceLock::new();

pub struct LogManager {
    settings: LogSettings,
    sender: Sender<LoggerCommandWrapper>,
    startup_time: Instant,
    counter: AtomicU64,
}

impl LogManager {
    pub fn instance() -> &'static Self {
        &MANAGER.get().unwrap()
    }

    pub fn initialize(settings: LogSettings) -> Result<JoinHandle<()>, ()> {
        let real_settings = validate_settings(settings);

        let (tx, rx) = channel();

        let mgr = LogManager {
            settings: real_settings,
            sender: tx,
            startup_time: Instant::now(),
            counter: AtomicU64::new(0),
        };
        MANAGER.set(mgr).map_err(|_| ())?;

        let join_handle = thread::spawn(move || {
            do_logging_loop(rx, MANAGER.get().unwrap());
        });
        Ok(join_handle)
    }

    pub fn deinitialize(&self) -> Result<(), ()> {
        self.request_halt()
    }

    pub(crate) fn stage_message(&self, channel: String, level: LogLevel, message: String)
                     -> Result<(), StagedMessage> {
        if level < self.settings.min_level {
            return Ok(());
        }

        let mono_time = Instant::now();
        let sys_time = SystemTime::now();
        let msg_obj = StagedMessage { channel, level, message, mono_time, sys_time };
        let cmd = LoggerCommandWrapper {
            command: LoggerCommand::EmitMessage(msg_obj),
            ordinal: self.next_ordinal(),
        };
        self.sender.send(cmd).map_err(|sent_cmd| {
            let LoggerCommand::EmitMessage(cmd) = sent_cmd.0.command
            else { panic!("Send error contained unrelated command object") };
            cmd
        })
    }

    fn next_ordinal(&self) -> u64 {
        self.counter.fetch_add(1, Ordering::Relaxed)
    }

    fn request_halt(&self) -> Result<(), ()> {
        let cmd = LoggerCommandWrapper {
            command: LoggerCommand::Halt,
            ordinal: self.next_ordinal(),
        };
        self.sender.send(cmd).map_err(|_| ())
    }
}

impl Drop for LogManager {
    fn drop(&mut self) {
        // don't care if it was already deinitialized
        _ = self.deinitialize();
    }
}

pub(crate) fn get_log_manager() -> Option<&'static LogManager> {
    MANAGER.get()
}


#[derive(Clone, Debug)]
pub(crate) struct StagedMessage {
    channel: String,
    level: LogLevel,
    message: String,
    mono_time: Instant,
    sys_time: SystemTime,
}

struct LoggerCommandWrapper {
    command: LoggerCommand,
    ordinal: u64,
}

#[derive(Clone, Debug)]
enum LoggerCommand {
    EmitMessage(StagedMessage),
    Halt,
}

fn emit_log_entry(mgr: &LogManager, message: StagedMessage) {
    if message.level < mgr.settings.min_level {
        return;
    }

    let prelude = mgr.settings.prelude.iter()
        .map(|c| match c {
            PreludeComponent::MonotonicTime => {
                let mono_elapsed = message.mono_time - mgr.startup_time;
                let mono_secs = mono_elapsed.as_secs_f64();
                let prec = mgr.settings.clock_precision;
                let width = prec + 5;
                format!("{:>width$.prec$}", mono_secs, width = width, prec = prec)
            },
            PreludeComponent::LocalTime => {
                DateTime::<Local>::from(message.sys_time)
                    .format("%Y-%m-%d %H:%M:%S")
                    .to_string()
            },
            PreludeComponent::UtcTime => {
                DateTime::<Local>::from(message.sys_time)
                    .to_utc()
                    .format("%Y-%m-%d %H:%M:%S")
                    .to_string()
            },
            PreludeComponent::Level => format!("[{}]", message.level.to_string().to_uppercase()),
            PreludeComponent::Channel => format!("[{}]", message.channel),
        })
        .collect::<Vec<_>>()
        .join(" ");

    let final_msg = if prelude.len() > 0 {
        format!("{} {}\n", prelude, message.message)
    } else {
        format!("{}\n", message.message)
    };

    let console_res = match message.level {
        LogLevel::Severe | LogLevel::Warning => {
            stderr().write_all(final_msg.as_bytes())
        },
        LogLevel::Info | LogLevel::Debug | LogLevel::Trace => {
            stdout().write_all(final_msg.as_bytes())
        },
    };

    if let Err(console_err) = console_res {
        eprintln!(
            "Failed to write log entry to stdout/stderr: {:?} (message: {:?})",
            console_err,
            final_msg
        );
    }
}

fn do_logging_loop(receiver: Receiver<LoggerCommandWrapper>, mgr: &'static LogManager) {
    let mut next_ordinal: u64 = 0;
    let mut buffered_cmds: HashMap<u64, LoggerCommand> = HashMap::new();
    loop {
        let incoming_cmd = receiver.recv().unwrap();
        if incoming_cmd.ordinal != next_ordinal {
            buffered_cmds.insert(incoming_cmd.ordinal, incoming_cmd.command);
            continue;
        }

        match incoming_cmd.command {
            LoggerCommand::EmitMessage(msg) => {
                emit_log_entry(mgr, msg);
            }
            LoggerCommand::Halt => {
                break;
            },
        }
        next_ordinal += 1;

        while let Some(cur_cmd) = buffered_cmds.remove(&next_ordinal) {
            match cur_cmd {
                LoggerCommand::EmitMessage(msg) => {
                    emit_log_entry(mgr, msg);
                }
                LoggerCommand::Halt => {
                    break;
                },
            }
            next_ordinal += 1;
        }
    }

    // flush remaining buffered message
    for (_, cmd) in buffered_cmds.into_iter().collect::<BTreeMap<_, _>>() {
        if let LoggerCommand::EmitMessage(msg) = cmd {
            emit_log_entry(mgr, msg);
        }
    }
}
