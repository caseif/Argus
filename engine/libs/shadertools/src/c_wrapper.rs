#[path = "./glslang/mod.rs"] mod glslang;

use core::ptr;
use libc::c_char;
use std::alloc::{alloc, dealloc, Layout};
use std::ffi::CStr;
use std::collections::HashMap;
use std::convert::TryInto;
use std::mem::{size_of, size_of_val};
use std::ptr::null;

use glslang::bindings::*;

#[repr(C)]
struct SizedByteArray {
    size: usize,
    data: [u8;0]
}

#[repr(C)]
struct SizedByteArrayWithIndex {
    size: usize,
    index: usize,
    data: [u8;0]
}

#[repr(C)]
pub struct InteropShaderCompilationResult {
    success: bool,
    shader_count: usize,
    stages: *const Stage,
    spirv_binaries: *const *const SizedByteArray,
    attrib_count: usize,
    attribs: *const u8,
    output_count: usize,
    outputs: *const u8,
    uniform_count: usize,
    uniforms: *const u8,
    buffer_count: usize,
    buffers: *const u8,
    ubo_count: usize,
    ubo_bindings: *const u8,
    ubo_names: *const u8,
}

unsafe fn copy_rust_map_to_interop_map_u32(map: &HashMap<String, u32>) -> *const u8 {
    let buf_len = map.iter()
        .map(|(name, _)| name.len() + size_of::<SizedByteArrayWithIndex>())
        .sum::<usize>() + size_of::<usize>();

    let interop_map = alloc(match Layout::array::<*const u8>(buf_len) {
        Ok(layout) => layout,
        Err(msg) => panic!("Failed to create interop map array layout: {}", msg)
    });

    *(interop_map as *mut usize) = buf_len;
    let mut off = size_of::<usize>();
    for (el_name, el_index) in map {
        let el_struct = &mut *(interop_map.offset(off as isize) as *mut SizedByteArrayWithIndex);
        off += el_name.len() + size_of::<SizedByteArrayWithIndex>();
        el_struct.size = el_name.len();
        el_struct.index = (*el_index) as usize;
        ptr::copy(el_name.as_ptr(), el_struct.data.as_mut_ptr(), el_name.len());
    }

    return interop_map;
}

unsafe fn copy_rust_map_to_interop_map_str(map: &HashMap<String, String>) -> *const u8 {
    let buf_len = map.iter()
        .map(|(k, v)| k.len() + v.len())
        .sum::<usize>() + size_of::<usize>() * 2;

    let interop_map = alloc(match Layout::array::<*const u8>(buf_len) {
        Ok(layout) => layout,
        Err(msg) => panic!("Failed to create interop map array layout: {}", msg)
    });

    *(interop_map as *mut usize) = buf_len;
    let mut off = size_of::<usize>();
    for (el_key, el_val) in map {
        let val_off = off + size_of::<usize>() * 2 + el_key.len();
        let el_struct = &mut *(interop_map.offset(off as isize) as *mut SizedByteArray);
        let el_k_struct = &mut *(el_struct.data.as_mut_ptr() as *mut SizedByteArray);
        let el_v_struct = &mut *(interop_map.offset(val_off as isize) as *mut SizedByteArray);
        el_struct.size = el_key.len() + el_val.len() + size_of::<SizedByteArray>() * 2;
        off += el_struct.size + size_of::<SizedByteArray>();
        el_k_struct.size = el_key.len();
        ptr::copy(el_key.as_ptr(), el_k_struct.data.as_mut_ptr(), el_key.len());
        el_v_struct.size = el_val.len();
        ptr::copy(el_val.as_ptr(), el_v_struct.data.as_mut_ptr(), el_val.len());
    }

    return interop_map;
}

unsafe fn dealloc_interop_map(map: *const u8) {
    let block_ptr = map;
    let alloc_size = *(block_ptr as *const usize);
    dealloc(block_ptr as *mut u8,
        match Layout::array::<*const u8>(alloc_size) {
            Ok(layout) => layout,
            Err(msg) => panic!("Failed to create SPIR-V attrib array layout: {}", msg)
        }
    );
}

#[no_mangle]
pub unsafe extern "C" fn transpile_glsl(stages: *const Stage, glsl_sources: *const *const c_char, count: usize,
        client: Client, client_version: TargetClientVersion, spirv_version: TargetLanguageVersion)
        -> *mut InteropShaderCompilationResult {
    let mut sources_map = HashMap::<Stage, String>::with_capacity(count.into());
    for i in 0..count {
        let stage = *stages.offset(i as isize);
        match CStr::from_ptr(*(glsl_sources.offset(i as isize) as *const *const c_char)).to_str() {
            Ok(src_str) => { sources_map.insert(stage, src_str.to_owned()); () },
            Err(msg) => panic!("{}", msg)
        }
    }

    let compile_res = match super::shadertools::compile_glsl_to_spirv(&sources_map, client,
            client_version, spirv_version) {
        Ok(v) => v,
        Err(e) => panic!("{}", e)
    };

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
        let shader_code = &compile_res.bytecode[&stage];

        let bin_data_len = shader_code.len();
        let bin_struct_len = size_of::<usize>() + bin_data_len;

        let bin_struct = alloc(match Layout::array::<*const u8>(bin_struct_len) {
            Ok(layout) => layout,
            Err(msg) => panic!("Failed to create SPIR-V binary array layout: {}", msg)
        }) as *mut SizedByteArray;

        *bin_ptr = bin_struct;

        *out_stages.offset(i.try_into().unwrap()) = stage;

        (*bin_struct).size = bin_data_len;
        ptr::copy(shader_code.as_ptr(), (*bin_struct).data.as_mut_ptr(), bin_data_len);

        *binaries.offset(i.try_into().unwrap()) = bin_struct;
    }

    (*res).stages = out_stages;
    (*res).spirv_binaries = binaries as *const *const SizedByteArray;

    (*res).attrib_count = compile_res.inputs.len();
    (*res).attribs = copy_rust_map_to_interop_map_u32(&compile_res.inputs);

    (*res).output_count = compile_res.outputs.len();
    (*res).outputs = copy_rust_map_to_interop_map_u32(&compile_res.outputs);

    (*res).uniform_count = compile_res.uniforms.len();
    (*res).uniforms = copy_rust_map_to_interop_map_u32(&compile_res.uniforms);

    (*res).buffer_count = compile_res.buffers.len();
    (*res).buffers = copy_rust_map_to_interop_map_u32(&compile_res.buffers);

    (*res).ubo_count = compile_res.ubo_bindings.len();
    (*res).ubo_bindings = copy_rust_map_to_interop_map_u32(&compile_res.ubo_bindings);
    (*res).ubo_names = copy_rust_map_to_interop_map_str(&compile_res.ubo_names);

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

    if (*result).attribs != null() {
        dealloc_interop_map((*result).attribs);
    }

    if (*result).outputs != null() {
        dealloc_interop_map((*result).outputs);
    }

    if (*result).uniforms != null() {
        dealloc_interop_map((*result).uniforms);
    }

    if (*result).buffers != null() {
        dealloc_interop_map((*result).buffers);
    }

    if (*result).ubo_bindings != null() {
        dealloc_interop_map((*result).ubo_bindings);
    }

    if (*result).ubo_names != null() {
        dealloc_interop_map((*result).ubo_names);
    }

    dealloc(result as *mut u8, Layout::new::<InteropShaderCompilationResult>());
}