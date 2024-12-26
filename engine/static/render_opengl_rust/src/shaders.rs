/*
 * This file is a part of Argus.
 * Copyright (c) 2019-2024, Max Roncace <mproncace@protonmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
use crate::aglet::*;
use crate::state::RendererState;
use crate::util::gl_util::*;
use render_rustabi::argus::render::*;
use resman_rustabi::argus::resman::{Resource, ResourceManager};
use shadertools::shadertools::compile_glsl_to_spirv;
use spirv_cross_sys::bindings::*;
use std::collections::HashMap;
use std::ffi::CString;
use std::{ffi, mem, ptr};
use lowlevel_rustabi::util::cstr_to_str;
use shadertools::glslang::{Client, Stage, TargetClientVersion, TargetLanguageVersion};

#[derive(Default)]
pub(crate) struct ShaderReflectionInfo {
    pub(crate) inputs: HashMap<String, u32>,
    pub(crate) outputs: HashMap<String, u32>,
    pub(crate) uniforms: HashMap<String, u32>,
    pub(crate) buffers: HashMap<String, u32>,
    pub(crate) ubo_bindings: HashMap<String, u32>,
    pub(crate) ubo_names: HashMap<String, String>,
}

pub(crate) struct LinkedProgram {
    pub(crate) handle: GlProgramHandle,
    pub(crate) reflection: ShaderReflectionInfo,
    pub(crate) has_custom_frag: bool,
}

fn compile_shaders(shaders: &Vec<Resource>) -> (Vec<GlShaderHandle>, ShaderReflectionInfo) {
    let mut compiled_handles: Vec<GlShaderHandle> = Vec::new();

    if shaders.is_empty() {
        return (Default::default(), Default::default());
    }

    let mut shader_uids: Vec<String> = Vec::new();
    let mut shader_sources: Vec<String> = Vec::new();
    for shader_res in shaders {
        let shader: &Shader = shader_res.get();
        shader_uids.push(shader.get_uid());
        shader_sources.push(String::from_utf8_lossy(shader.get_source()).to_string());
    }

    //TODO
    /*Logger::default_logger().debug(
        "Compiling SPIR-V from shader set [%s]",
        shader_uids.join(", ")
    );*/

    let compile_res = match compile_glsl_to_spirv(
        &shaders
            .iter()
            .map(|shader_res| {
                let shader: &Shader = shader_res.get();
                (
                    to_shadertools_stage(shader.get_stage()),
                    String::from_utf8(shader.get_source().to_vec()).unwrap(),
                )
            })
            .collect(),
        Client::OpenGL,
        TargetClientVersion::OpenGL450,
        TargetLanguageVersion::Spv1_0,
    ) {
        Ok(res) => res,
        Err(e) => {
            panic!("Failed to compile shaders: {e}");
        }
    };

    let refl_info = ShaderReflectionInfo {
        inputs: compile_res.inputs,
        outputs: compile_res.outputs,
        uniforms: compile_res.uniforms,
        buffers: compile_res.buffers,
        ubo_bindings: compile_res.ubo_bindings,
        ubo_names: compile_res.ubo_names,
    };

    let spirv_shaders = compile_res.bytecode;

    let have_gl_spirv = aglet_have_gl_version_4_1() && aglet_have_gl_arb_gl_spirv();

    let glsl_ver = if !have_gl_spirv {
        if aglet_have_gl_version_4_6() {
            460
        } else if aglet_have_gl_version_4_3() {
            430
        } else if aglet_have_gl_version_4_1() {
            410
        } else {
            330
        }
    } else {
        0
    };

    for (stage, spirv_src) in spirv_shaders {
        //TODO
        //Logger::default_logger().debug("Creating shader %s", shader.get_uid().c_str());

        let gl_shader_stage: GLuint = match stage {
            Stage::Vertex => GL_VERTEX_SHADER,
            Stage::Fragment => GL_FRAGMENT_SHADER,
            _ => {
                panic!("Unrecognized shader stage ordinal {:?}", stage);
            }
        };

        let shader_handle = glCreateShader(gl_shader_stage);
        if glIsShader(shader_handle) != GL_TRUE as u8 {
            panic!("Failed to create shader {}", glGetError());
        }

        if have_gl_spirv {
            //TODO
            //Logger::default_logger().debug("GL 4.1 profile and ARB_gl_spirv are available, "
            //        "passing compiled SPIR-V directly to OpenGL via glShaderBinary");

            glShaderBinary(
                1,
                &shader_handle,
                GL_SHADER_BINARY_FORMAT_SPIR_V_ARB,
                spirv_src.as_ptr().cast(),
                spirv_src.len() as GLsizei,
            );
            let entry_point_c = CString::new("main").unwrap();
            glSpecializeShaderARB(
                shader_handle,
                entry_point_c.as_ptr(),
                0,
                ptr::null(),
                ptr::null(),
            );
        } else {
            //TODO
            //Logger::default_logger().debug("GL 4.1 profile and/or ARB_gl_spirv is not available, "
            //       "transpiling compiled SPIR-V to GLSL");

            let spv_words: &[u32] = unsafe { mem::transmute(spirv_src.as_slice()) };
            let mut context: spvc_context = ptr::null_mut();
            let ctx_res = unsafe { spvc_context_create(&mut context) };
            if ctx_res != SPVC_SUCCESS {
                panic!(
                    "Failed to create SPIRV-Cross context: {}",
                    unsafe { cstr_to_str(spvc_context_get_last_error_string(context)) }
                );
            }

            let mut parsed_ir: spvc_parsed_ir = ptr::null_mut();
            let parse_res = unsafe {
                spvc_context_parse_spirv(
                    context,
                    spv_words.as_ptr(),
                    spv_words.len(),
                    &mut parsed_ir,
                )
            };
            if parse_res != SPVC_SUCCESS {
                panic!(
                    "Failed to parse SPIR-V words: {}",
                    unsafe { cstr_to_str(spvc_context_get_last_error_string(context)) }
                );
            }

            let mut compiler: spvc_compiler = ptr::null_mut();
            let compiler_res = unsafe {
                spvc_context_create_compiler(
                    context,
                    SPVC_BACKEND_GLSL,
                    parsed_ir,
                    SPVC_CAPTURE_MODE_COPY,
                    &mut compiler,
                )
            };
            if compiler_res != SPVC_SUCCESS {
                panic!(
                    "Failed to create SPIRV-Cross GLSL compiler: {}",
                    unsafe { cstr_to_str(spvc_context_get_last_error_string(context)) },
                );
            }

            let mut compiler_options: spvc_compiler_options = ptr::null_mut();
            let options_res = unsafe {
                spvc_compiler_create_compiler_options(compiler, &mut compiler_options)
            };
            if options_res != SPVC_SUCCESS {
                panic!(
                    "Failed to create SPIRV-Cross compiler options: {}",
                    unsafe { cstr_to_str(spvc_context_get_last_error_string(context)) },
                );
            }

            let set_ver_res = unsafe {
                spvc_compiler_options_set_uint(
                    compiler_options,
                    SPVC_COMPILER_OPTION_GLSL_VERSION,
                    glsl_ver,
                )
            };
            if set_ver_res != SPVC_SUCCESS {
                panic!(
                    "Failed to set GLSL version for SPIRV-Cross compiler: {}",
                    unsafe { cstr_to_str(spvc_context_get_last_error_string(context)) },
                );
            }

            let mut glsl_src_c: *const ffi::c_char = ptr::null_mut();
            let compile_res = unsafe { spvc_compiler_compile(compiler, &mut glsl_src_c) };
            if compile_res != SPVC_SUCCESS {
                panic!(
                    "Failed to transpile SPIR-V to GLSL: {}",
                    unsafe { cstr_to_str(spvc_context_get_last_error_string(context)) },
                );
            }

            //TODO
            //Logger::default_logger().debug("GLSL source:\n%s", glsl_src_c);

            let glsl_src_len = unsafe { ffi::CStr::from_ptr(glsl_src_c).count_bytes() as GLint };
            glShaderSource(shader_handle, 1, &glsl_src_c, &glsl_src_len);
            glCompileShader(shader_handle);
        }

        let mut gl_res: i32 = 0;
        glGetShaderiv(shader_handle, GL_COMPILE_STATUS, &mut gl_res);
        if gl_res == GL_FALSE as i32 {
            let mut log_len: i32 = 0;
            glGetShaderiv(shader_handle, GL_INFO_LOG_LENGTH, &mut log_len);
            assert!(log_len >= 0);
            let mut log_buf = vec![0u8; log_len as usize];
            glGetShaderInfoLog(
                shader_handle,
                log_len,
                ptr::null_mut(),
                log_buf.as_mut_ptr().cast(),
            );
            let stage_str = match stage {
                Stage::Vertex => "vertex",
                Stage::Fragment => "fragment",
                _ => "unknown",
            };

            let log_str = String::from_utf8(log_buf)
                .expect("OpenGL returned non-UTF-8 string for shader log??");
            panic!("Failed to compile {} shader: {}", stage_str, log_str);
        }

        compiled_handles.push(shader_handle);
    }

    (compiled_handles, refl_info)
}

pub(crate) fn link_program(shader_uids: &[&str]) -> LinkedProgram {
    let program_handle = glCreateProgram();
    if glIsProgram(program_handle) == GL_FALSE as u8 {
        panic!("Failed to create program: {}", glGetError());
    }

    let mut shaders: Vec<Resource> = Vec::new();
    let mut have_vert = false;
    let mut have_frag = false;
    for shader_uid in shader_uids {
        let shader_res = match ResourceManager::get_instance().get_resource(shader_uid) {
            Ok(r) => r,
            Err(e) => {
                panic!("Failed to load shader {shader_uid} ({:?})", e);
            }
        };

        shaders.push(shader_res);
        let shader: &Shader = shaders.last().unwrap().get();

        if shader.get_stage() == ShaderStage::Vertex {
            have_vert = true;
        } else if shader.get_stage() == ShaderStage::Fragment {
            have_frag = true;
        }
    }

    if !have_vert {
        let shader_res = match ResourceManager::get_instance().get_resource(SHADER_STD_VERT) {
            Ok(r) => r,
            Err(e) => {
                panic!("Failed to load built-in shader {SHADER_STD_VERT}: {:?}", e);
            }
        };

        shaders.push(shader_res);
    }
    if !have_frag {
        let shader_res = match ResourceManager::get_instance().get_resource(SHADER_STD_FRAG) {
            Ok(r) => r,
            Err(e) => {
                panic!("Failed to load built-in shader {SHADER_STD_FRAG}: {:?}", e);
            }
        };

        shaders.push(shader_res);
    }

    let (compiled_shaders, mut refl_info) = compile_shaders(&shaders);

    for handle in &compiled_shaders {
        glAttachShader(program_handle, *handle);
    }

    glBindFragDataLocation(program_handle, 0, SHADER_OUT_COLOR.as_ptr().cast());
    glBindFragDataLocation(program_handle, 1, SHADER_OUT_LIGHT_OPACITY.as_ptr().cast());

    glLinkProgram(program_handle);

    for handle in &compiled_shaders {
        glDetachShader(program_handle, *handle);
    }

    for shader_res in shaders {
        shader_res.release();
    }

    let mut res: i32 = 0;
    glGetProgramiv(program_handle, GL_LINK_STATUS, &mut res);
    if res == GL_FALSE as i32 {
        let mut log_len: i32 = 0;
        glGetProgramiv(program_handle, GL_INFO_LOG_LENGTH, &mut log_len);
        assert!(log_len >= 0);
        let mut log_buf = vec![0u8; log_len as usize];
        glGetProgramInfoLog(
            program_handle,
            log_len,
            ptr::null_mut(),
            log_buf.as_mut_ptr().cast(),
        );
        let log_str =
            String::from_utf8(log_buf).expect("OpenGL returned non-UTF-8 string for program log??");
        panic!("Failed to link program: {log_str}");
    }

    // need 410 support for attribute location decorations
    if !aglet_have_gl_version_4_1() {
        let mut attrib_max_len: GLint = 0;
        let mut attrib_count: GLint = 0;

        glGetProgramiv(
            program_handle,
            GL_ACTIVE_ATTRIBUTE_MAX_LENGTH,
            &mut attrib_max_len,
        );
        assert!(attrib_max_len >= 0);
        glGetProgramiv(program_handle, GL_ACTIVE_ATTRIBUTES, &mut attrib_count);

        let mut attrib_name_len: GLsizei = 0;
        let mut attrib_size: GLint = 0;
        let mut attrib_type: GLenum = 0;
        let mut attrib_name_buf = vec![0u8; attrib_max_len as usize];

        for i in 0..attrib_count as u32 {
            glGetActiveAttrib(
                program_handle,
                i,
                attrib_max_len,
                &mut attrib_name_len,
                &mut attrib_size,
                &mut attrib_type,
                attrib_name_buf.as_mut_ptr().cast(),
            );

            assert!(attrib_name_len <= attrib_max_len);
            let attrib_loc = glGetAttribLocation(program_handle, attrib_name_buf.as_ptr().cast());
            assert!(attrib_loc >= 0);
            refl_info.inputs.insert(
                String::from_utf8(attrib_name_buf.clone())
                    .expect("OpenGL returned non-UTF-8 string for attribute name??"),
                attrib_loc as u32,
            );
        }
    }

    // need 430 support for uniform location/binding decorations
    if !aglet_have_gl_version_4_3() {
        let mut uniform_max_len: GLint = 0;
        let mut uniform_count: GLint = 0;

        glGetProgramiv(
            program_handle,
            GL_ACTIVE_UNIFORM_MAX_LENGTH,
            &mut uniform_max_len,
        );
        assert!(uniform_max_len >= 0);
        glGetProgramiv(program_handle, GL_ACTIVE_UNIFORMS, &mut uniform_count);

        let mut uniform_name_len: GLsizei = 0;
        let mut uniform_name_buf = vec![0u8; uniform_max_len as usize];
        uniform_name_buf.resize(uniform_max_len as usize, 0);

        for i in 0..uniform_count as u32 {
            glGetActiveUniformName(
                program_handle,
                i,
                uniform_max_len,
                &mut uniform_name_len,
                uniform_name_buf.as_mut_ptr().cast(),
            );
            let uniform_name = String::from_utf8(uniform_name_buf.clone())
                .expect("OpenGL returned non-UTF-8 string as uniform name??");
            assert!(uniform_name_len <= uniform_max_len);
            let uniform_loc =
                glGetUniformLocation(program_handle, uniform_name_buf.as_ptr().cast());
            assert!(uniform_loc >= 0);
            refl_info.uniforms.insert(uniform_name, uniform_loc as u32);
        }
    }

    LinkedProgram {
        handle: program_handle,
        reflection: refl_info,
        has_custom_frag: have_frag,
    }
}

pub(crate) fn get_material_program<'a>(
    linked_programs: &'a mut HashMap<String, LinkedProgram>,
    material_res: &Resource,
) -> &'a LinkedProgram {
    linked_programs
        .entry(material_res.get_prototype().uid)
        .or_insert_with(|| {
            let material: Material = material_res.get_ffi();
            link_program(&material.get_shader_uids())
        })
}

pub(crate) fn deinit_shader(shader: GlShaderHandle) {
    glDeleteShader(shader);
}

pub(crate) fn remove_shader(state: &mut RendererState, shader_uid: &str) {
    //TODO
    //Logger::default_logger().debug("De-initializing shader {shader_uid}");
    if let Some(shader) = state.compiled_shaders.remove(shader_uid) {
        deinit_shader(shader);
    }
}

pub(crate) fn deinit_program(program: GlProgramHandle) {
    glDeleteProgram(program);
}

pub(crate) fn get_std_program(storage: &mut Option<LinkedProgram>) -> &LinkedProgram {
    storage.get_or_insert_with(|| link_program(&[SHADER_STD_VERT, SHADER_STD_FRAG]))
}

pub(crate) fn get_shadowmap_program(storage: &mut Option<LinkedProgram>) -> &LinkedProgram {
    storage.get_or_insert_with(|| link_program(&[SHADER_SHADOWMAP_VERT, SHADER_SHADOWMAP_FRAG]))
}

pub(crate) fn get_lighting_program(storage: &mut Option<LinkedProgram>) -> &LinkedProgram {
    storage.get_or_insert_with(|| link_program(&[SHADER_LIGHTING_VERT, SHADER_LIGHTING_FRAG]))
}

pub(crate) fn get_lightmap_composite_program(storage: &mut Option<LinkedProgram>) -> &LinkedProgram {
    storage.get_or_insert_with(|| {
        link_program(&[SHADER_LIGHTMAP_COMPOSITE_VERT, SHADER_LIGHTMAP_COMPOSITE_FRAG])
    })
}

fn to_shadertools_stage(stage: ShaderStage) -> Stage {
    match stage {
        ShaderStage::Vertex => Stage::Vertex,
        ShaderStage::Fragment => Stage::Fragment,
    }
}

#[no_mangle]
pub extern "C" fn link_program_2() {
    link_program(&["asdf"]);
}
