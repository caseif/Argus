#[path = "./glslang/mod.rs"] mod glslang;

use core::ptr;
use libc::c_char;
use std::alloc::{alloc, dealloc, Layout};
use std::ffi::{CStr, CString};
use std::collections::HashMap;
use std::convert::TryInto;
use std::mem::{size_of, size_of_val};
use std::ptr::null;
use std::str;

use glslang::{*, bindings::*};

#[repr(C)]
struct SizedByteArray {
    size: usize,
    data: u8
}

#[repr(C)]
struct SizedByteArrayWithIndex {
    size: usize,
    index: usize,
    data: u8
}

#[repr(C)]
pub struct InteropShaderCompilationResult {
    success: bool,
    shader_count: usize,
    stages: *const Stage,
    spirv_binaries: *const *const SizedByteArray,
    attrib_count: usize,
    attribs: *const *const SizedByteArrayWithIndex,
    uniform_count: usize,
    uniforms: *const *const SizedByteArrayWithIndex,
}

#[no_mangle]
pub unsafe extern "C" fn transpile_glsl(stages: *const Stage, glsl_sources: *const *const c_char, count: usize,
        client: Client, client_version: TargetClientVersion, spirv_version: TargetLanguageVersion)
        -> *mut InteropShaderCompilationResult {
    let mut sources_map = HashMap::<Stage, &str>::with_capacity(count.into());
    for i in 0..count {
        let stage = *stages.offset(i as isize);
        match CStr::from_ptr(*(glsl_sources.offset(i as isize) as *const *const c_char)).to_str() {
            Ok(src_str) => { sources_map.insert(stage, src_str); () },
            Err(msg) => panic!("{}", msg)
        }
    }

    let compile_res = compile_glsl_to_spirv_impl(&sources_map, client, client_version, spirv_version);

    let res = alloc(Layout::new::<InteropShaderCompilationResult>()) as *mut InteropShaderCompilationResult;
    (*res).shader_count = count;

    let out_stages = alloc(match Layout::array::<*const Stage>(count) {
        Ok(layout) => layout,
        Err(msg) => panic!("Failed to create SPIR-V stages array layout: {}", msg)
    }) as *mut Stage;

    let binaries = alloc(match Layout::array::<*const u8>(count) {
        Ok(layout) => layout,
        Err(msg) => panic!("Failed to create SPIR-V binaries array layout: {}", msg)
    }) as *mut *mut SizedByteArray;

    for i in 0..count {
        let stage = *stages.offset(i.try_into().unwrap());
        let bin_ptr = binaries.offset(i.try_into().unwrap());
        let shader_code = &compile_res[&stage];
        
        let bin_data_len = shader_code.len();
        let bin_struct_len = size_of::<usize>() + bin_data_len;

        let bin_struct = alloc(match Layout::array::<*const u8>(bin_struct_len) {
            Ok(layout) => layout,
            Err(msg) => panic!("Failed to create SPIR-V binary array layout: {}", msg)
        }) as *mut SizedByteArray;

        *bin_ptr = bin_struct;

        *out_stages.offset(i.try_into().unwrap()) = stage;

        (*bin_struct).size = bin_data_len;
        ptr::copy(shader_code.as_ptr(), &mut (*bin_struct).data, bin_data_len);

        *binaries.offset(i.try_into().unwrap()) = bin_struct;
    }

    (*res).stages = out_stages;
    (*res).spirv_binaries = binaries as *const *const SizedByteArray;

    //TODO: populate this later
    (*res).attrib_count = 0;
    (*res).attribs = null();
    (*res).uniform_count = 0;
    (*res).uniforms = null();

    (*res).success = true;

    return res;
}

#[no_mangle]
pub unsafe extern "C" fn free_compilation_result(result: *mut InteropShaderCompilationResult) {
    if !(*result).success {
        return;
    }

    let shader_count = (*result).shader_count;

    if shader_count > 0 {
        dealloc((*result).stages as *mut u8, match Layout::array::<*const Stage>(shader_count) {
            Ok(layout) => layout,
            Err(msg) => panic!("Failed to create SPIR-V stages array layout: {}", msg)
        });

        for i in 0..shader_count {
            let bin_struct_ptr = *(*result).spirv_binaries.offset(i.try_into().unwrap()) as *mut SizedByteArray;
            let bin_len = (*bin_struct_ptr).size;
            dealloc(bin_struct_ptr as *mut u8,
                match Layout::array::<*const u8>(bin_len + size_of_val(&(*bin_struct_ptr).size)) {
                    Ok(layout) => layout,
                    Err(msg) => panic!("Failed to create SPIR-V binary array layout: {}", msg)
                }
            );
        }

        dealloc((*result).spirv_binaries as *mut u8, match Layout::array::<*const u8>(shader_count) {
            Ok(layout) => layout,
            Err(msg) => panic!("Failed to create SPIR-V binaries array layout: {}", msg)
        });
    }

    if (*result).attrib_count > 0 {
        for i in 0..(*result).attrib_count {
            let attrib_struct_ptr = *(*result).attribs.offset(i.try_into().unwrap()) as *mut SizedByteArrayWithIndex;
            let attrib_name_len = (*attrib_struct_ptr).size;
            dealloc(attrib_struct_ptr as *mut u8,
                match Layout::array::<*const u8>(attrib_name_len + size_of_val(&(*attrib_struct_ptr).size)
                        + size_of_val(&(*attrib_struct_ptr).index)) {
                    Ok(layout) => layout,
                    Err(msg) => panic!("Failed to create SPIR-V attrib array layout: {}", msg)
                }
            );
        }

        dealloc((*result).attribs as *mut u8, match Layout::array::<*const u8>((*result).attrib_count) {
            Ok(layout) => layout,
            Err(msg) => panic!("Failed to create SPIR-V attrib array layout: {}", msg)
        });
    }

    if (*result).uniform_count > 0 {
        for i in 0..(*result).uniform_count {
            let uniform_struct_ptr = *(*result).uniforms.offset(i.try_into().unwrap()) as *mut SizedByteArrayWithIndex;
            let uniform_name_len = (*uniform_struct_ptr).size;
            dealloc(uniform_struct_ptr as *mut u8,
                match Layout::array::<*const u8>(uniform_name_len + size_of_val(&(*uniform_struct_ptr).size)
                        + size_of_val(&(*uniform_struct_ptr).index)) {
                    Ok(layout) => layout,
                    Err(msg) => panic!("Failed to create SPIR-V uniform array layout: {}", msg)
                }
            );
        }

        dealloc((*result).uniforms as *mut u8, match Layout::array::<*const u8>((*result).uniform_count) {
            Ok(layout) => layout,
            Err(msg) => panic!("Failed to create SPIR-V uniform array layout: {}", msg)
        });
    }

    dealloc(result as *mut u8, Layout::new::<InteropShaderCompilationResult>());
}

fn compile_glsl_to_spirv_impl(glsl_sources: &HashMap<Stage, &str>, client: Client, client_version: TargetClientVersion,
        spirv_version: TargetLanguageVersion) -> HashMap<Stage, Vec<u8>> {
    let mut res = HashMap::<Stage, Vec<u8>>::with_capacity(glsl_sources.len());

    for source in glsl_sources {
        let mut program = Program::create();
        
        let stage = *source.0;
        let glsl = source.1;
        let src_c_str = CString::new(glsl.as_bytes()).unwrap();

        glslang::initialize_process();

        let shader_messages = Messages::none();

        let input = &Input {
            language: Source::Glsl,
            stage: stage,
            client: client,
            client_version: client_version,
            target_language: TargetLanguage::Spv,
            target_language_version: spirv_version,
            /** Shader source code */
            code: src_c_str.as_ptr(),
            default_version: 0,
            default_profile: ProfileBit::Core.into(),
            force_default_version_and_profile: 0,
            forward_compatible: 0,
            messages: shader_messages,
            resource: &DEFAULT_BUILT_IN_RESOURCE,
        };

        let mut shader = Shader::create(input);
        shader.set_options(ShaderOptionBit::AutoMapBindings | ShaderOptionBit::AutoMapLocations);

        if !shader.preprocess(input) {
            panic!("Failed to preprocess shader for stage {}:\n{}\n{}", stage as i32,
                shader.get_info_log(), shader.get_info_debug_log());
        }

        if !shader.parse(input) {
            panic!("Failed to parse shader for stage {}:\n{}\n{}", stage as i32,
                shader.get_info_log(), shader.get_info_debug_log());
        }

        program.add_shader(shader);

        let program_messages = Messages::none();
        program.map_io();
        if !program.link(program_messages) {
            panic!("Failed to link shader program:\n{}\n{}",
                program.get_info_log(), program.get_info_debug_log());
        }
        
        let spirv_options = SpvOptions {
            generate_debug_info: false, // this absolutely must not be true or it'll cause glSpecializeShader to fail
            strip_debug_info: false,
            disable_optimizer: true,
            optimize_size: false,
            disassemble: false,
            validate: false,
        };

        let stage = *source.0;
        program.spirv_generate_with_options(stage, &spirv_options);
        res.insert(stage, program.spirv_get());
    }

    return res;
}
