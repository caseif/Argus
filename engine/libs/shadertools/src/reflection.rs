use std::collections::HashMap;

#[derive(Clone, Debug, Default)]
#[must_use]
pub struct ShaderReflectionInfo {
    pub attribute_locations: HashMap<String, u32>,
    pub output_locations: HashMap<String, u32>,
    pub uniform_variable_locations: HashMap<String, u32>,
    pub buffer_locations: HashMap<String, u32>,
    pub ubo_bindings: HashMap<String, u32>,
    pub ubo_instance_names: HashMap<String, String>,
}

impl ShaderReflectionInfo {
    #[must_use]
    pub fn has_attr(&self, name: impl AsRef<str>) -> bool {
        self.attribute_locations.contains_key(name.as_ref())
    }

    #[must_use]
    pub fn get_attr_loc(&self, name: impl AsRef<str>) -> Option<u32> {
        self.attribute_locations.get(name.as_ref()).copied()
    }

    pub fn get_attr_loc_and_then<F: FnOnce(u32)>(&self, name: impl AsRef<str>, f: F) {
        if let Some(&loc) = self.attribute_locations.get(name.as_ref()) {
            f(loc)
        }
    }

    pub fn set_attr_loc(&mut self, name: impl AsRef<str>, loc: u32) {
        self.attribute_locations.insert(name.as_ref().to_owned(), loc);
    }

    #[must_use]
    pub fn has_output(&self, name: impl AsRef<str>) -> bool {
        self.output_locations.contains_key(name.as_ref())
    }

    #[must_use]
    pub fn get_output_loc(&self, name: impl AsRef<str>) -> Option<u32> {
        self.output_locations.get(name.as_ref()).copied()
    }

    pub fn get_output_loc_and_then<F: FnOnce(u32)>(&self, name: impl AsRef<str>, f: F) {
        if let Some(&loc) = self.output_locations.get(name.as_ref()) {
            f(loc)
        }
    }

    pub fn set_output_loc(&mut self, name: impl AsRef<str>, loc: u32) {
        self.output_locations.insert(name.as_ref().to_owned(), loc);
    }

    #[must_use]
    pub fn has_uniform(&self, ubo: impl AsRef<str>, name: impl AsRef<str>) -> bool {
        self.ubo_instance_names.get(ubo.as_ref())
            .map(|inst_name| {
                let joined_name = format!("{}.{}", inst_name, name.as_ref());
                self.ubo_bindings.contains_key(joined_name.as_str())
            })
            .unwrap_or(false)
    }

    #[must_use]
    pub fn has_uniform_var(&self, name: impl AsRef<str>) -> bool {
        self.uniform_variable_locations.contains_key(name.as_ref())
    }

    #[must_use]
    pub fn get_uniform_loc(&self, ubo: impl AsRef<str>, name: impl AsRef<str>) -> Option<u32> {
        self.ubo_instance_names.get(ubo.as_ref()).and_then(|inst_name| {
            let joined_name = format!("{}.{}", inst_name, name.as_ref());
            self.uniform_variable_locations.get(&joined_name).copied()
        })
    }

    #[must_use]
    pub fn get_uniform_var_loc(&self, name: impl AsRef<str>) -> Option<u32> {
        self.uniform_variable_locations.get(name.as_ref()).copied()
    }

    pub fn get_uniform_loc_and_then<F: FnOnce(u32)>(
        &self,
        ubo: impl AsRef<str>,
        name: impl AsRef<str>,
        f: F
    ) {
        if let Some(inst_name) = self.get_ubo_instance_name(ubo) {
            let joined_name = format!("{}.{}", inst_name, name.as_ref());
            if let Some(&loc) = self.uniform_variable_locations.get(&joined_name) {
                f(loc);
            }
        }
    }

    pub fn get_uniform_var_loc_and_then<F: FnOnce(u32)>(&self, name: impl AsRef<str>, f: F) {
        if let Some(&loc) = self.uniform_variable_locations.get(name.as_ref()) {
            f(loc)
        }
    }

    pub fn set_uniform_loc(&mut self, ubo: impl AsRef<str>, name: impl AsRef<str>, loc: u32)
                           -> Result<(), &'static str> {
        if let Some(inst_name) = self.get_ubo_instance_name(ubo) {
            let joined_name = format!("{}.{}", inst_name, name.as_ref());
            self.uniform_variable_locations.insert(joined_name, loc);
            Ok(())
        } else {
            Err("Tried to set location for uniform variable in non-existent buffer")
        }
    }

    pub fn set_uniform_var_loc(&mut self, name: impl AsRef<str>, loc: u32) {
        self.uniform_variable_locations.insert(name.as_ref().to_owned(), loc);
    }

    #[must_use]
    pub fn has_ubo(&self, name: impl AsRef<str>) -> bool {
        self.ubo_bindings.contains_key(name.as_ref())
    }

    #[must_use]
    pub fn get_ubo_bindings(&self) -> &HashMap<String, u32> {
        &self.ubo_bindings
    }

    #[must_use]
    pub fn get_ubo_binding(&self, name: impl AsRef<str>) -> Option<u32> {
        self.ubo_bindings.get(name.as_ref()).copied()
    }

    pub fn get_ubo_binding_and_then<F: FnOnce(u32)>(&self, name: impl AsRef<str>, f: F) {
        if let Some(&loc) = self.ubo_bindings.get(name.as_ref()) {
            f(loc);
        }
    }

    pub fn set_ubo_binding(&mut self, name: impl AsRef<str>, loc: u32) {
        self.ubo_bindings.insert(name.as_ref().to_owned(), loc);
    }

    #[must_use]
    pub fn get_ubo_instance_name(&self, name: impl AsRef<str>) -> Option<&str> {
        self.ubo_instance_names.get(name.as_ref()).map(|s| s.as_str())
    }

    pub fn set_ubo_instance_name(
        &mut self,
        ubo_name: impl AsRef<str>,
        instance_name: impl AsRef<str>
    ) {
        self.ubo_instance_names.insert(
            ubo_name.as_ref().to_owned(),
            instance_name.as_ref().to_owned()
        );
    }
}
