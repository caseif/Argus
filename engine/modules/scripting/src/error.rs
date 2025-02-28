use std::error::Error;
use std::fmt::{Display, Formatter};
use num_enum::{IntoPrimitive, TryFromPrimitive};

#[derive(Clone, Copy, Debug)]
pub enum SymbolType {
    Type,
    Enum,
    EnumValue,
    Field,
    Function,
}

#[derive(Clone, Debug)]
pub struct BindingError {
    pub ty: BindingErrorType,
    pub bound_name: String,
    pub msg: String,
}

impl BindingError {
    pub fn new(ty: BindingErrorType, bound_name: impl Into<String>, msg: impl Into<String>)
        -> Self {
        Self {
            ty,
            bound_name: bound_name.into(),
            msg: msg.into(),
        }
    }
}

#[derive(Clone, Debug, PartialEq, IntoPrimitive, TryFromPrimitive)]
#[repr(u32)]
pub enum BindingErrorType {
    DuplicateName,
    ConflictingName,
    InvalidDefinition,
    InvalidMembers,
    UnknownParent,
    Other,
}

#[derive(Clone, Debug)]
pub struct SymbolNotBoundError {
    pub symbol_type: SymbolType,
    pub symbol_name: String,
}

impl SymbolNotBoundError {
    pub fn new(symbol_type: SymbolType, symbol_name: impl Into<String>) -> Self {
        Self { symbol_type, symbol_name: symbol_name.into() }
    }
}

#[derive(Debug)]
pub struct ScriptLoadError {
    pub resource_uid: String,
    pub msg: String,
}

impl Display for ScriptLoadError {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(f, "Failed to load script {}: {}", self.resource_uid, self.msg)
    }
}

impl Error for ScriptLoadError {}

impl ScriptLoadError {
    pub fn new(resource_uid: impl Into<String>, msg: impl Into<String>) -> Self {
        Self { resource_uid: resource_uid.into(), msg: msg.into() }
    }
}
