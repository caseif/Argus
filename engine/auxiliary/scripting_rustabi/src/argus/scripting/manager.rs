use crate::argus::scripting::{BoundEnumDef, BoundFunctionDef, BoundTypeDef};
use crate::scripting_cabi::*;

pub struct ScriptManager {
    handle: argus_script_manager_t,
}

impl ScriptManager {
    pub fn instance() -> Self {
        let handle = unsafe { argus_script_manager_instance() };
        Self { handle }
    }

    pub fn bind_type(&self, def: BoundTypeDef) {
        unsafe { argus_script_manager_bind_type(self.handle, def.get_handle()); }
    }

    pub fn bind_enum(&self, def: BoundEnumDef) {
        unsafe { argus_script_manager_bind_enum(self.handle, def.get_handle()); }
    }

    pub fn bind_global_function(&self, def: BoundFunctionDef) {
        unsafe { argus_script_manager_bind_global_function(self.handle, def.get_handle()); }
    }
}
