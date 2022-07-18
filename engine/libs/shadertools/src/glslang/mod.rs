#![allow(dead_code)]

pub mod bindings;

use self::bindings::*;
use std::ffi::{CStr, CString};

pub fn initialize_process() -> i32 {
    unsafe {
        return glslang_initialize_process();
    }
}

pub fn finalize_process() {
    unsafe {
        glslang_finalize_process();
    }
}

pub struct Shader {
    handle: ShaderHandle
}

impl Shader {
    pub fn create(input: &Input) -> Shader {
        unsafe {
            return Shader { handle: glslang_shader_create(input) };
        }
    }

    pub fn shift_binding(&mut self, res: ResourceType, base: u32) {
        unsafe {
            glslang_shader_shift_binding(self.handle, res, base);
        }
    }

    pub fn shift_binding_for_set(&mut self, res: ResourceType, base: u32, set: u32) {
        unsafe {
            glslang_shader_shift_binding_for_set(self.handle, res, base, set);
        }
    }

    pub fn set_options(&mut self, options: ShaderOptions) {
        unsafe {
            glslang_shader_set_options(self.handle, options);
        }
    }

    pub fn set_glsl_version(&mut self, version: i32) {
        unsafe {
            glslang_shader_set_glsl_version(self.handle, version);
        }
    }

    pub fn preprocess(&mut self, input: &Input) -> bool {
        unsafe {
            return glslang_shader_preprocess(self.handle, input) != 0;
        }
    }

    pub fn parse(&mut self, input: &Input) -> bool {
        unsafe {
            return glslang_shader_parse(self.handle, input) != 0;
        }
    }

    pub fn get_preprocessed_code<'a>(&mut self) -> &'a str {
        unsafe {
            return CStr::from_ptr(glslang_shader_get_preprocessed_code(self.handle)).to_str().unwrap();
        }
    }

    pub fn get_info_log<'a>(&mut self) -> &'a str {
        unsafe {
            return CStr::from_ptr(glslang_shader_get_info_log(self.handle)).to_str().unwrap();
        }
    }

    pub fn get_info_debug_log<'a>(&mut self) -> &'a str {
        unsafe {
            return CStr::from_ptr(glslang_shader_get_info_debug_log(self.handle)).to_str().unwrap();
        }
    }
}

impl Drop for Shader {
    fn drop(&mut self) {
        unsafe {
            glslang_shader_delete(self.handle);
        }
    }
}

pub struct Program {
    handle: ProgramHandle,
    shaders: Vec<Shader>
}

impl Program {
    pub fn create() -> Program {
        unsafe {
            return Program { handle: glslang_program_create(), shaders: Vec::<Shader>::new() };
        }
    }

    pub fn add_shader(&mut self, shader: Shader) -> &Shader {
        // need to take ownership so it doesn't get freed prematurely
        self.shaders.push(shader);
        let shader_moved = self.shaders.last().unwrap();
        unsafe {
            glslang_program_add_shader(self.handle, shader_moved.handle);
        }
        return &shader_moved;
    }

    pub fn link(&mut self, messages: Messages) -> bool {
        unsafe {
            return glslang_program_link(self.handle, messages) != 0;
        }
    }

    pub fn add_source_text(&mut self, stage: Stage, text: &str, len: usize) {
        unsafe {
            let c_str = CString::new(text).unwrap();
            glslang_program_add_source_text(self.handle, stage, c_str.as_ptr(), len);
        }
    }

    pub fn set_source_file(&mut self, stage: Stage, file: &str) {
        unsafe {
            let c_str = CString::new(file).unwrap();
            glslang_program_set_source_file(self.handle, stage, c_str.as_ptr());
        }
    }

    pub fn map_io(&mut self) -> bool {
        unsafe {
            return glslang_program_map_io(self.handle) != 0;
        }
    }

    pub fn spirv_generate(&mut self, stage: Stage) {
        unsafe {
            glslang_program_SPIRV_generate(self.handle, stage);
        }
    }

    pub fn spirv_generate_with_options(&mut self, stage: Stage, spv_options: &SpvOptions) {
        unsafe {
            let mut options_copy = spv_options.clone();
            glslang_program_SPIRV_generate_with_options(self.handle, stage, &mut options_copy);
        }
    }

    pub fn spirv_get(&mut self) -> Vec<u8> {
        unsafe {
            let len = glslang_program_SPIRV_get_size(self.handle);
            let ptr = glslang_program_SPIRV_get_ptr(self.handle) as *mut u8;

            // reinterpret u32 array as u8 array 4 times as large
            let mut res = Vec::<u8>::new();
            res.extend_from_slice(std::slice::from_raw_parts(ptr, len * 4));
            return res;
        }
    }

    pub fn spirv_get_messages<'a>(&mut self) -> &'a str {
        unsafe {
            return CStr::from_ptr(glslang_program_SPIRV_get_messages(self.handle)).to_str().unwrap();
        }
    }

    pub fn get_info_log<'a>(&mut self) -> &'a str {
        unsafe {
            return CStr::from_ptr(glslang_program_get_info_log(self.handle)).to_str().unwrap();
        }
    }

    pub fn get_info_debug_log<'a>(&mut self) -> &'a str {
        unsafe {
            return CStr::from_ptr(glslang_program_get_info_debug_log(self.handle)).to_str().unwrap();
        }
    }
}

impl Drop for Program {
    fn drop(&mut self) {
        unsafe {
            glslang_program_delete(self.handle);
        }
    }
}
