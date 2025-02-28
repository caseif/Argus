use std::error::Error;
use std::fmt::{Debug, Display, Formatter};

#[derive(Clone, Debug)]
pub struct ArgumentError {
    message: String,
    argument_name: String,
}

impl Display for ArgumentError {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(f, "Invalid value passed for argument '{}': {}", self.argument_name, self.message)
    }
}

impl Error for ArgumentError {}

impl ArgumentError {
    pub fn new(argument_name: impl Into<String>, message: impl Into<String>) -> ArgumentError {
        Self { argument_name: argument_name.into(), message: message.into() }
    }

    pub fn get_argument_name(&self) -> &str {
        self.argument_name.as_str()
    }

    pub fn get_message(&self) -> &str {
        self.message.as_str()
    }
}

#[macro_export]
macro_rules! arg_error {
    ($var: ident, $msg: literal) => {
        {
            let _ = $var;
            ::std::result::Result::Err($crate::error::ArgumentError::new(stringify!($var), $msg))
        }
    }
}

#[macro_export]
macro_rules! validate_arg {
    ($var: ident, $cond: expr, $msg: literal) => {
        {
            if !($cond) {
                return $crate::arg_error!($var, $msg);
            }
        }
    }
}
