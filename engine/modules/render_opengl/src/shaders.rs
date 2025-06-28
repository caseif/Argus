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
use std::collections::HashMap;
use std::ffi::CString;
use std::ptr;
use glslang::{OpenGlVersion, SpirvVersion};
use spirv_cross2::{Compiler, Module};
use spirv_cross2::compile::CompilableTarget;
use spirv_cross2::compile::glsl::GlslVersion;
use spirv_cross2::targets::Glsl;
use argus_logging::debug;
use argus_render::common::{Material, Shader, ShaderStage};
use argus_render::constants::*;
use argus_resman::{Resource, ResourceIdentifier, ResourceManager};
use argus_shadertools::glslang::Target;
use argus_shadertools::shadertools::compile_glsl_to_spirv;
use crate::LOGGER;

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
        let shader: &Shader = shader_res.get().unwrap();
        shader_uids.push(shader.get_uid().to_string());
        shader_sources.push(String::from_utf8_lossy(shader.get_source()).to_string());
    }

    debug!(LOGGER, "Compiling SPIR-V from shader set [{}]", shader_uids.join(", "));

    let compile_res = match compile_glsl_to_spirv(
        &shaders
            .iter()
            .map(|shader_res| {
                let shader: &Shader = shader_res.get().unwrap();
                (
                    to_shadertools_stage(shader.get_stage()),
                    String::from_utf8(shader.get_source().to_vec()).unwrap(),
                )
            })
            .collect(),
        Target::OpenGL {
            version: OpenGlVersion::OpenGL4_5,
            spirv_version: Some(SpirvVersion::SPIRV1_0),
        },
        450,
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
        Some(
            if aglet_have_gl_version_4_6() {
                GlslVersion::Glsl460
            } else if aglet_have_gl_version_4_3() {
                GlslVersion::Glsl430
            } else if aglet_have_gl_version_4_1() {
                GlslVersion::Glsl410
            } else {
                GlslVersion::Glsl330
            }
        )
    } else {
        None
    };

    for (i, (stage, spirv_src)) in spirv_shaders.iter().enumerate() {
        debug!(LOGGER, "Building shader {}", shader_uids[i]);

        let gl_shader_stage: GLuint = match stage {
            glslang::ShaderStage::Vertex => GL_VERTEX_SHADER,
            glslang::ShaderStage::Fragment => GL_FRAGMENT_SHADER,
            _ => {
                panic!("Unrecognized shader stage ordinal {:?}", stage);
            }
        };

        let shader_handle = glCreateShader(gl_shader_stage);
        if glIsShader(shader_handle) != GL_TRUE as u8 {
            panic!("Failed to create shader {}", glGetError());
        }

        if have_gl_spirv {
            debug!(
                LOGGER,
                "GL 4.1 profile and ARB_gl_spirv are available, \
                 passing compiled SPIR-V directly to OpenGL via glShaderBinary",
            );

            _ = glGetError(); // clear error
            glShaderBinary(
                1,
                &shader_handle,
                GL_SHADER_BINARY_FORMAT_SPIR_V_ARB,
                spirv_src.as_ptr().cast(),
                (spirv_src.len() * size_of::<u32>()) as GLsizei,
            );

            if let Some(gl_err) = get_gl_error() {
                panic!("glShaderBinary failed with error {gl_err}");
            }

            let entry_point_c = CString::new("main").unwrap();
            glSpecializeShaderARB(
                shader_handle,
                entry_point_c.as_ptr().cast(),
                0,
                ptr::null(),
                ptr::null(),
            );
        } else {
            debug!(LOGGER, "GL 4.1 profile and/or ARB_gl_spirv is not available, transpiling compiled SPIR-V to GLSL");

            let module = Module::from_words(spirv_src.as_slice());
            let compiler = Compiler::<Glsl>::new(module).expect("Failed to create SPIR-V context");

            let mut options = Glsl::options();
            options.version = glsl_ver.unwrap();
            
            let compile_res = compiler.compile(&options).expect("Failed to transpile SPIR-V");
            let glsl_src: &str = compile_res.as_ref();
            let glsl_src_c = CString::new(glsl_src).unwrap();

            debug!(LOGGER, "GLSL source:\n{}", glsl_src);

            let glsl_src_len = glsl_src.len() as GLint;
            glShaderSource(shader_handle, 1, &glsl_src_c.as_ptr().cast(), &glsl_src_len);
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
                glslang::ShaderStage::Vertex => "vertex",
                glslang::ShaderStage::Fragment => "fragment",
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

pub(crate) fn link_program(shader_uids: impl IntoIterator<Item = impl AsRef<str>>) -> LinkedProgram {
    let program_handle = glCreateProgram();
    if glIsProgram(program_handle) == GL_FALSE as u8 {
        panic!("Failed to create program: {}", glGetError());
    }

    let mut shaders: Vec<Resource> = Vec::new();
    let mut have_vert = false;
    let mut have_frag = false;
    for shader_uid in shader_uids {
        let shader_res = match ResourceManager::instance().get_resource(shader_uid.as_ref()) {
            Ok(r) => r,
            Err(e) => {
                panic!("Failed to load shader {} ({:?})", shader_uid.as_ref(), e);
            }
        };

        let shader: &Shader = shader_res.get().unwrap();

        if shader.get_stage() == ShaderStage::Vertex {
            have_vert = true;
        } else if shader.get_stage() == ShaderStage::Fragment {
            have_frag = true;
        }

        shaders.push(shader_res);
    }

    if !have_vert {
        let shader_res = match ResourceManager::instance().get_resource(SHADER_STD_VERT) {
            Ok(r) => r,
            Err(e) => {
                panic!("Failed to load built-in shader {SHADER_STD_VERT}: {:?}", e);
            }
        };

        shaders.push(shader_res);
    }
    if !have_frag {
        let shader_res = match ResourceManager::instance().get_resource(SHADER_STD_FRAG) {
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

    let out_color_name_c = CString::new(SHADER_OUT_COLOR).unwrap();
    let out_light_opac_name_c = CString::new(SHADER_OUT_LIGHT_OPACITY).unwrap();
    glBindFragDataLocation(program_handle, 0, out_color_name_c.as_ptr().cast());
    glBindFragDataLocation(program_handle, 1, out_light_opac_name_c.as_ptr().cast());

    glLinkProgram(program_handle);

    for handle in &compiled_shaders {
        glDetachShader(program_handle, *handle);
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
    linked_programs: &'a mut HashMap<ResourceIdentifier, LinkedProgram>,
    material_res: &Resource,
) -> &'a LinkedProgram {
    linked_programs
        .entry(material_res.get_prototype().uid.clone())
        .or_insert_with(|| {
            let material: &Material = material_res.get().unwrap();
            link_program(material.get_shader_uids())
        })
}

pub(crate) fn deinit_shader(shader: GlShaderHandle) {
    glDeleteShader(shader);
}

pub(crate) fn remove_shader(state: &mut RendererState, shader_uid: &str) {
    debug!(LOGGER, "De-initializing shader {shader_uid}");
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

fn to_shadertools_stage(stage: ShaderStage) -> glslang::ShaderStage {
    match stage {
        ShaderStage::Vertex => glslang::ShaderStage::Vertex,
        ShaderStage::Fragment => glslang::ShaderStage::Fragment,
    }
}
