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

#![allow(dead_code)]

use std::{ffi, mem};
use crate::bindings::*;

#[derive(Copy, Clone, Debug, Eq, PartialEq, Hash)]
pub enum Stage {
    Vertex = GLSLANG_STAGE_VERTEX as isize,
    TessControl = GLSLANG_STAGE_TESSCONTROL as isize,
    TessEvaluation = GLSLANG_STAGE_TESSEVALUATION as isize,
    Geometry = GLSLANG_STAGE_GEOMETRY as isize,
    Fragment = GLSLANG_STAGE_FRAGMENT as isize,
    Compute = GLSLANG_STAGE_COMPUTE as isize,
    Raygen = GLSLANG_STAGE_RAYGEN as isize,
    Intersect = GLSLANG_STAGE_INTERSECT as isize,
    AnyHit = GLSLANG_STAGE_ANYHIT as isize,
    ClosestHit = GLSLANG_STAGE_CLOSESTHIT as isize,
    Miss = GLSLANG_STAGE_MISS as isize,
    Callable = GLSLANG_STAGE_CALLABLE as isize,
    Task = GLSLANG_STAGE_TASK as isize,
    Mesh = GLSLANG_STAGE_MESH as isize,
}

#[derive(Copy, Clone, Debug, Eq, PartialEq, Hash)]
pub enum StageMask {
    Vertex = GLSLANG_STAGE_VERTEX_MASK as isize,
    TessControl = GLSLANG_STAGE_TESSCONTROL_MASK as isize,
    TessEvaluation = GLSLANG_STAGE_TESSEVALUATION_MASK as isize,
    Geometry = GLSLANG_STAGE_GEOMETRY_MASK as isize,
    Fragment = GLSLANG_STAGE_FRAGMENT_MASK as isize,
    Compute = GLSLANG_STAGE_COMPUTE_MASK as isize,
    Raygen = GLSLANG_STAGE_RAYGEN_MASK as isize,
    Intersect = GLSLANG_STAGE_INTERSECT_MASK as isize,
    AnyHit = GLSLANG_STAGE_ANYHIT_MASK as isize,
    ClosestHit = GLSLANG_STAGE_CLOSESTHIT_MASK as isize,
    Miss = GLSLANG_STAGE_MISS_MASK as isize,
    Callable = GLSLANG_STAGE_CALLABLE_MASK as isize,
    Task = GLSLANG_STAGE_TASK_MASK as isize,
    Mesh = GLSLANG_STAGE_MESH_MASK as isize,
}

#[derive(Copy, Clone, Debug, Eq, PartialEq, Hash)]
pub enum Source {
    None = GLSLANG_SOURCE_NONE as isize,
    Glsl = GLSLANG_SOURCE_GLSL as isize,
    Hlsl = GLSLANG_SOURCE_HLSL as isize,
}

#[derive(Copy, Clone, Debug, Eq, PartialEq, Hash)]
pub enum Client {
    None = GLSLANG_CLIENT_NONE as isize,
    Vulkan = GLSLANG_CLIENT_VULKAN as isize,
    OpenGL = GLSLANG_CLIENT_OPENGL as isize,
}

#[derive(Copy, Clone, Debug, Eq, PartialEq, Hash)]
pub enum TargetLanguage {
    None = GLSLANG_TARGET_NONE as isize,
    Spv = GLSLANG_TARGET_SPV as isize,
}

#[derive(Copy, Clone, Debug, Eq, PartialEq, Hash)]
pub enum TargetClientVersion {
    Vulkan1_0 = GLSLANG_TARGET_VULKAN_1_0 as isize,
    Vulkan1_1 = GLSLANG_TARGET_VULKAN_1_1 as isize,
    Vulkan1_2 = GLSLANG_TARGET_VULKAN_1_2 as isize,
    Vulkan1_3 = GLSLANG_TARGET_VULKAN_1_3 as isize,
    OpenGL450 = GLSLANG_TARGET_OPENGL_450 as isize,
}

#[derive(Copy, Clone, Debug, Eq, PartialEq, Hash)]
pub enum TargetLanguageVersion {
    Spv1_0 = GLSLANG_TARGET_SPV_1_0 as isize,
    Spv1_1 = GLSLANG_TARGET_SPV_1_1 as isize,
    Spv1_2 = GLSLANG_TARGET_SPV_1_2 as isize,
    Spv1_3 = GLSLANG_TARGET_SPV_1_3 as isize,
    Spv1_4 = GLSLANG_TARGET_SPV_1_4 as isize,
    Spv1_5 = GLSLANG_TARGET_SPV_1_5 as isize,
    Spv1_6 = GLSLANG_TARGET_SPV_1_6 as isize,
}

#[derive(Copy, Clone, Debug, Eq, PartialEq, Hash)]
pub enum Executable {
    VertexFragment = GLSLANG_EX_VERTEX_FRAGMENT as isize,
    Fragment = GLSLANG_EX_FRAGMENT as isize,
}

#[derive(Copy, Clone, Debug, Eq, PartialEq, Hash)]
pub enum OptimizationLevel {
    NoGeneration = GLSLANG_OPT_NO_GENERATION as isize,
    None = GLSLANG_OPT_NONE as isize,
    Simple = GLSLANG_OPT_SIMPLE as isize,
    Full = GLSLANG_OPT_FULL as isize,
}

#[derive(Copy, Clone, Debug, Eq, PartialEq, Hash)]
pub enum TextureSamplerTransformMode {
    Keep = GLSLANG_TEX_SAMP_TRANS_KEEP as isize,
    RemoveSampler = GLSLANG_TEX_SAMP_TRANS_UPGRADE_TEXTURE_REMOVE_SAMPLER as isize,
}

#[derive(Copy, Clone, Debug, Eq, PartialEq, Hash)]
pub enum Messages {
    Default = GLSLANG_MSG_DEFAULT_BIT as isize,
    RelaxedErrors = GLSLANG_MSG_RELAXED_ERRORS_BIT as isize,
    SuppressWarnings = GLSLANG_MSG_SUPPRESS_WARNINGS_BIT as isize,
    Ast = GLSLANG_MSG_AST_BIT as isize,
    SpvRules = GLSLANG_MSG_SPV_RULES_BIT as isize,
    VulkanRules = GLSLANG_MSG_VULKAN_RULES_BIT as isize,
    OnlyPreprocessor = GLSLANG_MSG_ONLY_PREPROCESSOR_BIT as isize,
    ReadHlsl = GLSLANG_MSG_READ_HLSL_BIT as isize,
    CascadingErrors = GLSLANG_MSG_CASCADING_ERRORS_BIT as isize,
    KeepUncalled = GLSLANG_MSG_KEEP_UNCALLED_BIT as isize,
    HlslOffsets = GLSLANG_MSG_HLSL_OFFSETS_BIT as isize,
    DebugInfo = GLSLANG_MSG_DEBUG_INFO_BIT as isize,
    HlslEnable16bitTypes = GLSLANG_MSG_HLSL_ENABLE_16BIT_TYPES_BIT as isize,
    HlslLegalization = GLSLANG_MSG_HLSL_LEGALIZATION_BIT as isize,
    HlslDx9Compatible = GLSLANG_MSG_HLSL_DX9_COMPATIBLE_BIT as isize,
    BuiltinSymbolTable = GLSLANG_MSG_BUILTIN_SYMBOL_TABLE_BIT as isize,
    Enhanced = GLSLANG_MSG_ENHANCED as isize,
}

#[derive(Copy, Clone, Debug, Eq, PartialEq, Hash)]
pub enum ReflectionOptions {
    Default = GLSLANG_REFLECTION_DEFAULT_BIT as isize,
    StrictArraySuffix = GLSLANG_REFLECTION_STRICT_ARRAY_SUFFIX_BIT as isize,
    BasicArraySuffix = GLSLANG_REFLECTION_BASIC_ARRAY_SUFFIX_BIT as isize,
    IntermediateIoo = GLSLANG_REFLECTION_INTERMEDIATE_IOO_BIT as isize,
    SeparateBuffers = GLSLANG_REFLECTION_SEPARATE_BUFFERS_BIT as isize,
    AllBlockVariables = GLSLANG_REFLECTION_ALL_BLOCK_VARIABLES_BIT as isize,
    UnwrapIoBlocks = GLSLANG_REFLECTION_UNWRAP_IO_BLOCKS_BIT as isize,
    AllIoVariables = GLSLANG_REFLECTION_ALL_IO_VARIABLES_BIT as isize,
    SharedStd140Ssbo = GLSLANG_REFLECTION_SHARED_STD140_SSBO_BIT as isize,
    SharedStd140Ubo = GLSLANG_REFLECTION_SHARED_STD140_UBO_BIT as isize,
}

#[derive(Copy, Clone, Debug, Eq, PartialEq, Hash)]
pub enum Profile {
    Bad = GLSLANG_BAD_PROFILE as isize,
    None = GLSLANG_NO_PROFILE as isize,
    Core = GLSLANG_CORE_PROFILE as isize,
    Compatibility = GLSLANG_COMPATIBILITY_PROFILE as isize,
    Es = GLSLANG_ES_PROFILE as isize,
}

#[derive(Copy, Clone, Debug, Eq, PartialEq, Hash)]
pub enum ShaderOptions {
    Default = GLSLANG_SHADER_DEFAULT_BIT as isize,
    AutoMapBindings = GLSLANG_SHADER_AUTO_MAP_BINDINGS as isize,
    AutoMapLocations = GLSLANG_SHADER_AUTO_MAP_LOCATIONS as isize,
    VulkanRulesRelaxed = GLSLANG_SHADER_VULKAN_RULES_RELAXED as isize,
}

#[derive(Copy, Clone, Debug, Eq, PartialEq, Hash)]
pub enum ResourceType {
    Sampler = GLSLANG_RESOURCE_TYPE_SAMPLER as isize,
    Texture = GLSLANG_RESOURCE_TYPE_TEXTURE as isize,
    Image = GLSLANG_RESOURCE_TYPE_IMAGE as isize,
    Ubo = GLSLANG_RESOURCE_TYPE_UBO as isize,
    Ssbo = GLSLANG_RESOURCE_TYPE_SSBO as isize,
    Uav = GLSLANG_RESOURCE_TYPE_UAV as isize,
}

pub type Limits = glslang_limits_t;

#[repr(C)]
#[derive(Copy, Clone, Debug)]
pub struct Resource {
    pub max_lights: i32,
    pub max_clip_planes: i32,
    pub max_texture_units: i32,
    pub max_texture_coords: i32,
    pub max_vertex_attribs: i32,
    pub max_vertex_uniform_components: i32,
    pub max_varying_floats: i32,
    pub max_vertex_texture_image_units: i32,
    pub max_combined_texture_image_units: i32,
    pub max_texture_image_units: i32,
    pub max_fragment_uniform_components: i32,
    pub max_draw_buffers: i32,
    pub max_vertex_uniform_vectors: i32,
    pub max_varying_vectors: i32,
    pub max_fragment_uniform_vectors: i32,
    pub max_vertex_output_vectors: i32,
    pub max_fragment_input_vectors: i32,
    pub min_program_texel_offset: i32,
    pub max_program_texel_offset: i32,
    pub max_clip_distances: i32,
    pub max_compute_work_group_count_x: i32,
    pub max_compute_work_group_count_y: i32,
    pub max_compute_work_group_count_z: i32,
    pub max_compute_work_group_size_x: i32,
    pub max_compute_work_group_size_y: i32,
    pub max_compute_work_group_size_z: i32,
    pub max_compute_uniform_components: i32,
    pub max_compute_texture_image_units: i32,
    pub max_compute_image_uniforms: i32,
    pub max_compute_atomic_counters: i32,
    pub max_compute_atomic_counter_buffers: i32,
    pub max_varying_components: i32,
    pub max_vertex_output_components: i32,
    pub max_geometry_input_components: i32,
    pub max_geometry_output_components: i32,
    pub max_fragment_input_components: i32,
    pub max_image_units: i32,
    pub max_combined_image_units_and_fragment_outputs: i32,
    pub max_combined_shader_output_resources: i32,
    pub max_image_samples: i32,
    pub max_vertex_image_uniforms: i32,
    pub max_tess_control_image_uniforms: i32,
    pub max_tess_evaluation_image_uniforms: i32,
    pub max_geometry_image_uniforms: i32,
    pub max_fragment_image_uniforms: i32,
    pub max_combined_image_uniforms: i32,
    pub max_geometry_texture_image_units: i32,
    pub max_geometry_output_vertices: i32,
    pub max_geometry_total_output_components: i32,
    pub max_geometry_uniform_components: i32,
    pub max_geometry_varying_components: i32,
    pub max_tess_control_input_components: i32,
    pub max_tess_control_output_components: i32,
    pub max_tess_control_texture_image_units: i32,
    pub max_tess_control_uniform_components: i32,
    pub max_tess_control_total_output_components: i32,
    pub max_tess_evaluation_input_components: i32,
    pub max_tess_evaluation_output_components: i32,
    pub max_tess_evaluation_texture_image_units: i32,
    pub max_tess_evaluation_uniform_components: i32,
    pub max_tess_patch_components: i32,
    pub max_patch_vertices: i32,
    pub max_tess_gen_level: i32,
    pub max_viewports: i32,
    pub max_vertex_atomic_counters: i32,
    pub max_tess_control_atomic_counters: i32,
    pub max_tess_evaluation_atomic_counters: i32,
    pub max_geometry_atomic_counters: i32,
    pub max_fragment_atomic_counters: i32,
    pub max_combined_atomic_counters: i32,
    pub max_atomic_counter_bindings: i32,
    pub max_vertex_atomic_counter_buffers: i32,
    pub max_tess_control_atomic_counter_buffers: i32,
    pub max_tess_evaluation_atomic_counter_buffers: i32,
    pub max_geometry_atomic_counter_buffers: i32,
    pub max_fragment_atomic_counter_buffers: i32,
    pub max_combined_atomic_counter_buffers: i32,
    pub max_atomic_counter_buffer_size: i32,
    pub max_transform_feedback_buffers: i32,
    pub max_transform_feedback_interleaved_components: i32,
    pub max_cull_distances: i32,
    pub max_combined_clip_and_cull_distances: i32,
    pub max_samples: i32,
    pub max_mesh_output_vertices_nv: i32,
    pub max_mesh_output_primitives_nv: i32,
    pub max_mesh_work_group_size_x_nv: i32,
    pub max_mesh_work_group_size_y_nv: i32,
    pub max_mesh_work_group_size_z_nv: i32,
    pub max_task_work_group_size_x_nv: i32,
    pub max_task_work_group_size_y_nv: i32,
    pub max_task_work_group_size_z_nv: i32,
    pub max_mesh_view_count_nv: i32,
    pub max_mesh_output_vertices_ext: i32,
    pub max_mesh_output_primitives_ext: i32,
    pub max_mesh_work_group_size_x_ext: i32,
    pub max_mesh_work_group_size_y_ext: i32,
    pub max_mesh_work_group_size_z_ext: i32,
    pub max_task_work_group_size_x_ext: i32,
    pub max_task_work_group_size_y_ext: i32,
    pub max_task_work_group_size_z_ext: i32,
    pub max_mesh_view_count_ext: i32,
    pub max_dual_source_draw_buffers_ext: i32,
    pub limits: Limits,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct IncludeResult {
    pub header_name: *const ffi::c_char,
    pub header_data: *const ffi::c_char,
    pub header_length: usize,
}

pub type GlslIncludeLocalFunc = extern "C" fn(
    ctx: *mut ffi::c_void,
    header_name: *const ffi::c_char,
    includer_name: *const ffi::c_char,
    include_depth: usize,
) -> *mut IncludeResult;

pub type GlslIncludeSystemFunc = extern "C" fn(
    ctx: *mut ffi::c_void,
    header_name: *const ffi::c_char,
    includer_name: *const ffi::c_char,
    include_depth: usize,
) -> *mut IncludeResult;

pub type GlslFreeIncludeResultFunc =
extern "C" fn(ctx: *mut ffi::c_void, result: *mut IncludeResult) -> ffi::c_int;

#[repr(C)]
#[derive(Clone, Copy)]
pub struct IncludeCallbacks {
    pub include_system: Option<GlslIncludeSystemFunc>,
    pub include_local: Option<GlslIncludeLocalFunc>,
    pub free_include_result: Option<GlslFreeIncludeResultFunc>,
}

#[derive(Clone)]
pub struct Input {
    pub language: Source,
    pub stage: Stage,
    pub client: Client,
    pub client_version: TargetClientVersion,
    pub target_language: TargetLanguage,
    pub target_language_version: TargetLanguageVersion,
    pub code: ffi::CString,
    pub default_version: i32,
    pub default_profile: Profile,
    pub force_default_version_and_profile: i32,
    pub forward_compatible: i32,
    pub messages: Messages,
    pub resource: Resource,
    pub callbacks: IncludeCallbacks,
    pub callbacks_ctx: *mut ffi::c_void,
}

impl Into<glslang_input_t> for &Input {
    fn into(self) -> glslang_input_t {
        glslang_input_t {
            language: self.language as glslang_target_language_t,
            stage: self.stage as glslang_stage_t,
            client: self.client as glslang_client_t,
            client_version: self.client_version as glslang_target_client_version_t,
            target_language: self.target_language as glslang_target_language_t,
            target_language_version:
                self.target_language_version as glslang_target_language_version_t,
            code: self.code.as_ptr(),
            default_version: self.default_version as ffi::c_int,
            default_profile: self.default_profile as glslang_profile_t,
            force_default_version_and_profile: self.force_default_version_and_profile as ffi::c_int,
            forward_compatible: self.forward_compatible as ffi::c_int,
            messages: self.messages as glslang_messages_t,
            resource: unsafe { mem::transmute(&self.resource) },
            callbacks: unsafe { mem::transmute(self.callbacks) },
            callbacks_ctx: self.callbacks_ctx,
        }
    }
}

pub type SpvOptions = glslang_spv_options_t;

pub fn initialize_process() -> i32 {
    unsafe { glslang_initialize_process() }
}

pub fn finalize_process() {
    unsafe { glslang_finalize_process(); }
}

pub struct Shader {
    handle: *mut glslang_shader_t,
}

impl Shader {
    pub fn create(input: &Input) -> Shader {
        unsafe {
            Shader {
                handle: glslang_shader_create(&input.into()),
            }
        }
    }

    pub fn shift_binding(&mut self, res: ResourceType, base: u32) {
        unsafe { glslang_shader_shift_binding(self.handle, res as glslang_resource_type_t, base); }
    }

    pub fn shift_binding_for_set(&mut self, res: ResourceType, base: u32, set: u32) {
        unsafe { glslang_shader_shift_binding_for_set(
            self.handle,
            res as glslang_resource_type_t,
            base,
            set);
        }
    }

    pub fn set_options(&mut self, options: ShaderOptions) {
        unsafe { glslang_shader_set_options(self.handle, options as ffi::c_int); }
    }

    pub fn set_glsl_version(&mut self, version: i32) {
        unsafe { glslang_shader_set_glsl_version(self.handle, version); }
    }

    pub fn preprocess(&mut self, input: &Input) -> bool {
        unsafe { glslang_shader_preprocess(self.handle, &input.into()) != 0 }
    }

    pub fn parse(&mut self, input: &Input) -> bool {
        unsafe { glslang_shader_parse(self.handle, &input.into()) != 0 }
    }

    pub fn get_preprocessed_code<'a>(&mut self) -> &'a str {
        unsafe {
            ffi::CStr::from_ptr(glslang_shader_get_preprocessed_code(self.handle))
                .to_str()
                .unwrap()
        }
    }

    pub fn get_info_log<'a>(&mut self) -> &'a str {
        unsafe {
            ffi::CStr::from_ptr(glslang_shader_get_info_log(self.handle))
                .to_str()
                .unwrap()
        }
    }

    pub fn get_info_debug_log<'a>(&mut self) -> &'a str {
        unsafe {
            ffi::CStr::from_ptr(glslang_shader_get_info_debug_log(self.handle))
                .to_str()
                .unwrap()
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
    handle: *mut glslang_program_t,
    shaders: Vec<Shader>,
}

impl Program {
    pub fn create() -> Program {
        unsafe {
            Program {
                handle: glslang_program_create(),
                shaders: Vec::<Shader>::new(),
            }
        }
    }

    pub fn add_shader(&mut self, shader: Shader) -> &Shader {
        // need to take ownership so it doesn't get freed prematurely
        self.shaders.push(shader);
        let shader_moved = self.shaders.last().unwrap();
        unsafe {
            glslang_program_add_shader(self.handle, shader_moved.handle);
        }
        &shader_moved
    }

    pub fn link(&mut self, messages: Messages) -> bool {
        unsafe { glslang_program_link(self.handle, messages as ffi::c_int) != 0 }
    }

    pub fn add_source_text(&mut self, stage: Stage, text: &str, len: usize) {
        unsafe {
            let c_str = ffi::CString::new(text).unwrap();
            glslang_program_add_source_text(
                self.handle,
                stage as glslang_stage_t,
                c_str.as_ptr(),
                len
            );
        }
    }

    pub fn set_source_file(&mut self, stage: Stage, file: &str) {
        unsafe {
            let c_str = ffi::CString::new(file).unwrap();
            glslang_program_set_source_file(self.handle, stage as glslang_stage_t, c_str.as_ptr());
        }
    }

    pub fn map_io(&mut self) -> bool {
        unsafe { glslang_program_map_io(self.handle) != 0 }
    }

    pub fn spirv_generate(&mut self, stage: Stage) {
        unsafe {
            glslang_program_SPIRV_generate(self.handle, stage as glslang_stage_t);
        }
    }

    pub fn spirv_generate_with_options(&mut self, stage: Stage, spv_options: &SpvOptions) {
        unsafe {
            let mut options_copy = spv_options.clone();
            glslang_program_SPIRV_generate_with_options(
                self.handle,
                stage as glslang_stage_t,
                &mut options_copy
            );
        }
    }

    pub fn spirv_get(&mut self) -> Vec<u8> {
        unsafe {
            let len = glslang_program_SPIRV_get_size(self.handle);
            let ptr = glslang_program_SPIRV_get_ptr(self.handle) as *mut u8;

            // reinterpret u32 array as u8 array 4 times as large
            let mut res = Vec::<u8>::new();
            res.extend_from_slice(std::slice::from_raw_parts(ptr, len * 4));

            res
        }
    }

    pub fn spirv_get_messages<'a>(&mut self) -> &'a str {
        unsafe {
            ffi::CStr::from_ptr(glslang_program_SPIRV_get_messages(self.handle))
                .to_str()
                .unwrap()
        }
    }

    pub fn get_info_log<'a>(&mut self) -> &'a str {
        unsafe {
             ffi::CStr::from_ptr(glslang_program_get_info_log(self.handle))
                .to_str()
                .unwrap()
        }
    }

    pub fn get_info_debug_log<'a>(&mut self) -> &'a str {
        unsafe {
            ffi::CStr::from_ptr(glslang_program_get_info_debug_log(self.handle))
                .to_str()
                .unwrap()
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


pub const DEFAULT_BUILT_IN_RESOURCE: Resource = Resource {
    max_lights: 32,
    max_clip_planes: 6,
    max_texture_units: 32,
    max_texture_coords: 32,
    max_vertex_attribs: 64,
    max_vertex_uniform_components: 4096,
    max_varying_floats: 64,
    max_vertex_texture_image_units: 32,
    max_combined_texture_image_units: 80,
    max_texture_image_units: 32,
    max_fragment_uniform_components: 4096,
    max_draw_buffers: 32,
    max_vertex_uniform_vectors: 128,
    max_varying_vectors: 8,
    max_fragment_uniform_vectors: 16,
    max_vertex_output_vectors: 16,
    max_fragment_input_vectors: 15,
    min_program_texel_offset: -8,
    max_program_texel_offset: 7,
    max_clip_distances: 8,
    max_compute_work_group_count_x: 65535,
    max_compute_work_group_count_y: 65535,
    max_compute_work_group_count_z: 65535,
    max_compute_work_group_size_x: 1024,
    max_compute_work_group_size_y: 1024,
    max_compute_work_group_size_z: 64,
    max_compute_uniform_components: 1024,
    max_compute_texture_image_units: 16,
    max_compute_image_uniforms: 8,
    max_compute_atomic_counters: 8,
    max_compute_atomic_counter_buffers: 1,
    max_varying_components: 60,
    max_vertex_output_components: 64,
    max_geometry_input_components: 64,
    max_geometry_output_components: 128,
    max_fragment_input_components: 128,
    max_image_units: 8,
    max_combined_image_units_and_fragment_outputs: 8,
    max_combined_shader_output_resources: 8,
    max_image_samples: 0,
    max_vertex_image_uniforms: 0,
    max_tess_control_image_uniforms: 0,
    max_tess_evaluation_image_uniforms: 0,
    max_geometry_image_uniforms: 0,
    max_fragment_image_uniforms: 8,
    max_combined_image_uniforms: 8,
    max_geometry_texture_image_units: 16,
    max_geometry_output_vertices: 256,
    max_geometry_total_output_components: 1024,
    max_geometry_uniform_components: 1024,
    max_geometry_varying_components: 64,
    max_tess_control_input_components: 128,
    max_tess_control_output_components: 128,
    max_tess_control_texture_image_units: 16,
    max_tess_control_uniform_components: 1024,
    max_tess_control_total_output_components: 4096,
    max_tess_evaluation_input_components: 128,
    max_tess_evaluation_output_components: 128,
    max_tess_evaluation_texture_image_units: 16,
    max_tess_evaluation_uniform_components: 1024,
    max_tess_patch_components: 120,
    max_patch_vertices: 32,
    max_tess_gen_level: 64,
    max_viewports: 16,
    max_vertex_atomic_counters: 0,
    max_tess_control_atomic_counters: 0,
    max_tess_evaluation_atomic_counters: 0,
    max_geometry_atomic_counters: 0,
    max_fragment_atomic_counters: 8,
    max_combined_atomic_counters: 8,
    max_atomic_counter_bindings: 1,
    max_vertex_atomic_counter_buffers: 0,
    max_tess_control_atomic_counter_buffers: 0,
    max_tess_evaluation_atomic_counter_buffers: 0,
    max_geometry_atomic_counter_buffers: 0,
    max_fragment_atomic_counter_buffers: 1,
    max_combined_atomic_counter_buffers: 1,
    max_atomic_counter_buffer_size: 16384,
    max_transform_feedback_buffers: 4,
    max_transform_feedback_interleaved_components: 64,
    max_cull_distances: 8,
    max_combined_clip_and_cull_distances: 8,
    max_samples: 4,
    max_mesh_output_vertices_nv: 256,
    max_mesh_output_primitives_nv: 512,
    max_mesh_work_group_size_x_nv: 32,
    max_mesh_work_group_size_y_nv: 1,
    max_mesh_work_group_size_z_nv: 1,
    max_task_work_group_size_x_nv: 32,
    max_task_work_group_size_y_nv: 1,
    max_task_work_group_size_z_nv: 1,
    max_mesh_view_count_nv: 4,
    max_mesh_output_vertices_ext: 256,
    max_mesh_output_primitives_ext: 256,
    max_mesh_work_group_size_x_ext: 128,
    max_mesh_work_group_size_y_ext: 128,
    max_mesh_work_group_size_z_ext: 128,
    max_task_work_group_size_x_ext: 128,
    max_task_work_group_size_y_ext: 128,
    max_task_work_group_size_z_ext: 128,
    max_mesh_view_count_ext: 4,
    max_dual_source_draw_buffers_ext: 1,
    limits: Limits {
        non_inductive_for_loops: true,
        while_loops: true,
        do_while_loops: true,
        general_uniform_indexing: true,
        general_attribute_matrix_vector_indexing: true,
        general_varying_indexing: true,
        general_sampler_indexing: true,
        general_variable_indexing: true,
        general_constant_matrix_vector_indexing: true,
    },
};
