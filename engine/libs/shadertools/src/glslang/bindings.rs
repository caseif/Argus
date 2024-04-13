#![allow(dead_code)]

use std::os::raw::{c_char, c_int, c_uint, c_void};

use bitmask::bitmask;

/* glslang_c_shader_types.c */

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum Stage {
    Vertex,
    Tesscontrol,
    Tessevaluation,
    Geometry,
    Fragment,
    Compute,
    Raygen,
    Intersect,
    Anyhit,
    Closesthit,
    Miss,
    Callable,
    Task,
    Mesh,
} // would be better as stage, but this is ancient now

/* EShLanguageMask counterpart */
bitmask! {
    #[repr(C)]
    pub mask StageMask: i32 where flags StageMaskBit {
        Vertex = (1 << (Stage::Vertex as i32)),
        Tesscontrol = (1 << (Stage::Tesscontrol as i32)),
        Tessevaluation = (1 << (Stage::Tessevaluation as i32)),
        Geometry = (1 << (Stage::Geometry as i32)),
        Fragment = (1 << (Stage::Fragment as i32)),
        Compute = (1 << (Stage::Compute as i32)),
        Raygen = (1 << (Stage::Raygen as i32)),
        Intersect = (1 << (Stage::Intersect as i32)),
        Anyhit = (1 << (Stage::Anyhit as i32)),
        Closesthit = (1 << (Stage::Closesthit as i32)),
        Miss = (1 << (Stage::Miss as i32)),
        Callable = (1 << (Stage::Callable as i32)),
        Task = (1 << (Stage::Task as i32)),
        Mesh = (1 << (Stage::Mesh as i32)),
    }
}

/* EShSource counterpart */
#[repr(C)]
#[derive(Clone, Copy, PartialEq, Eq, Hash)]
pub enum Source {
    None,
    Glsl,
    Hlsl
}

/* EShClient counterpart */
#[repr(C)]
#[derive(Clone, Copy, PartialEq, Eq, Hash)]
pub enum Client {
    None,
    Vulkan,
    Opengl
}

/* EShTargetLanguage counterpart */
#[repr(C)]
#[derive(Clone, Copy, PartialEq, Eq, Hash)]
pub enum TargetLanguage {
    None,
    Spv
}

/* SH_TARGET_ClientVersion counterpart */
#[repr(C)]
#[derive(Clone, Copy, PartialEq, Eq, Hash)]
pub enum TargetClientVersion {
    Vulkan1_0 = (1 << 22),
    Vulkan1_1 = (1 << 22) | (1 << 12),
    Vulkan1_2 = (1 << 22) | (2 << 12),
    Vulkan1_3 = (1 << 22) | (3 << 12),
    Opengl450 = 450
}

/* SH_TARGET_LanguageVersion counterpart */
#[repr(C)]
#[derive(Clone, Copy, PartialEq, Eq, Hash)]
pub enum TargetLanguageVersion {
    Spv1_0 = (1 << 16),
    Spv1_1 = (1 << 16) | (1 << 8),
    Spv1_2 = (1 << 16) | (2 << 8),
    Spv1_3 = (1 << 16) | (3 << 8),
    Spv1_4 = (1 << 16) | (4 << 8),
    Spv1_5 = (1 << 16) | (5 << 8),
    Spv1_6 = (1 << 16) | (6 << 8)
}

/* EShExecutable counterpart */
#[repr(C)]
#[derive(Clone, Copy, PartialEq, Eq, Hash)]
pub enum Executable {
    VertexFragment,
    Fragment
}

// EShOptimizationLevel counterpart
// This enum is not used in the current C interface, but could be added at a later date.
// GLSLANG_OPT_NONE is the current default.
#[repr(C)]
#[derive(Clone, Copy, PartialEq, Eq, Hash)]
pub enum OptimizationLevel {
    NoGeneration,
    None,
    Simple,
    Full
}

/* EShTextureSamplerTransformMode counterpart */
#[repr(C)]
#[derive(Clone, Copy, PartialEq, Eq, Hash)]
pub enum TextureSamplerTransformMode {
    Keep,
    UpgradeTextureRemoveSampler
}

/* EShMessages counterpart */
bitmask! {
    #[repr(C)]
    pub mask Messages: i32 where flags MessagesBit {
        RelaxedErrorsBit = (1 << 0),
        SuppressWarningsBit = (1 << 1),
        AstBit = (1 << 2),
        SpvRulesBit = (1 << 3),
        VulkanRulesBit = (1 << 4),
        OnlyPreprocessorBit = (1 << 5),
        ReadHlslBit = (1 << 6),
        CascadingErrorsBit = (1 << 7),
        KeepUncalledBit = (1 << 8),
        HlslOffsetsBit = (1 << 9),
        DebugInfoBit = (1 << 10),
        HlslEnable16bitTypesBit = (1 << 11),
        HlslLegalizationBit = (1 << 12),
        HlslDx9CompatibleBit = (1 << 13),
        BuiltinSymbolTableBit = (1 << 14),
        Enhanced = (1 << 15)
    }
}

/* EShReflectionOptions counterpart */
bitmask! {
    #[repr(C)]
    pub mask ReflectionOptions: i32 where flags ReflectionOptionBit {
        StrictArraySuffixBit = (1 << 0),
        BasicArraySuffixBit = (1 << 1),
        IntermediateIooBit = (1 << 2),
        SeparateBuffersBit = (1 << 3),
        AllBlockVariablesBit = (1 << 4),
        UnwrapIoBlocksBit = (1 << 5),
        AllIoVariablesBit = (1 << 6),
        SharedStd140SsboBit = (1 << 7),
        SharedStd140UboBit = (1 << 8)
    }
}

/* EProfile counterpart (from Versions.h) */
bitmask! {
    #[repr(C)]
    pub mask Profile: i32 where flags ProfileBit {
        None = (1 << 0),
        Core = (1 << 1),
        Compatibility = (1 << 2),
        Es = (1 << 3)
    }
}

/* Shader options */
bitmask! {
    #[repr(C)]
    pub mask ShaderOptions: i32 where flags ShaderOptionBit {
        AutoMapBindings = (1 << 0),
        AutoMapLocations = (1 << 1),
        VulkanRulesRelaxed = (1 << 2)
    }
}

/* TResourceType counterpart */
#[repr(C)]
#[derive(Clone, Copy, PartialEq, Eq, Hash)]
pub enum ResourceType {
    Sampler,
    Texture,
    Image,
    Ubo,
    Ssbo,
    Uav
}

/* /glslang_c_shader_types.h */

/* glslang_c_interface.h */

pub type ShaderHandle = *mut c_void;
pub type ProgramHandle = *mut c_void;

/* TLimits counterpart */
#[repr(C)]
#[derive(Clone, Copy)]
pub struct Limits {
    pub non_inductive_for_loops: bool,
    pub while_loops: bool,
    pub do_while_loops: bool,
    pub general_uniform_indexing: bool,
    pub general_attribute_matrix_vector_indexing: bool,
    pub general_varying_indexing: bool,
    pub general_sampler_indexing: bool,
    pub general_variable_indexing: bool,
    pub general_constant_matrix_vector_indexing: bool
}

/* TBuiltInResource counterpart */
#[repr(C)]
#[derive(Clone, Copy)]
pub struct Resource {
    pub max_lights: c_int,
    pub max_clip_planes: c_int,
    pub max_texture_units: c_int,
    pub max_texture_coords: c_int,
    pub max_vertex_attribs: c_int,
    pub max_vertex_uniform_components: c_int,
    pub max_varying_floats: c_int,
    pub max_vertex_texture_image_units: c_int,
    pub max_combined_texture_image_units: c_int,
    pub max_texture_image_units: c_int,
    pub max_fragment_uniform_components: c_int,
    pub max_draw_buffers: c_int,
    pub max_vertex_uniform_vectors: c_int,
    pub max_varying_vectors: c_int,
    pub max_fragment_uniform_vectors: c_int,
    pub max_vertex_output_vectors: c_int,
    pub max_fragment_input_vectors: c_int,
    pub min_program_texel_offset: c_int,
    pub max_program_texel_offset: c_int,
    pub max_clip_distances: c_int,
    pub max_compute_work_group_count_x: c_int,
    pub max_compute_work_group_count_y: c_int,
    pub max_compute_work_group_count_z: c_int,
    pub max_compute_work_group_size_x: c_int,
    pub max_compute_work_group_size_y: c_int,
    pub max_compute_work_group_size_z: c_int,
    pub max_compute_uniform_components: c_int,
    pub max_compute_texture_image_units: c_int,
    pub max_compute_image_uniforms: c_int,
    pub max_compute_atomic_counters: c_int,
    pub max_compute_atomic_counter_buffers: c_int,
    pub max_varying_components: c_int,
    pub max_vertex_output_components: c_int,
    pub max_geometry_input_components: c_int,
    pub max_geometry_output_components: c_int,
    pub max_fragment_input_components: c_int,
    pub max_image_units: c_int,
    pub max_combined_image_units_and_fragment_outputs: c_int,
    pub max_combined_shader_output_resources: c_int,
    pub max_image_samples: c_int,
    pub max_vertex_image_uniforms: c_int,
    pub max_tess_control_image_uniforms: c_int,
    pub max_tess_evaluation_image_uniforms: c_int,
    pub max_geometry_image_uniforms: c_int,
    pub max_fragment_image_uniforms: c_int,
    pub max_combined_image_uniforms: c_int,
    pub max_geometry_texture_image_units: c_int,
    pub max_geometry_output_vertices: c_int,
    pub max_geometry_total_output_components: c_int,
    pub max_geometry_uniform_components: c_int,
    pub max_geometry_varying_components: c_int,
    pub max_tess_control_input_components: c_int,
    pub max_tess_control_output_components: c_int,
    pub max_tess_control_texture_image_units: c_int,
    pub max_tess_control_uniform_components: c_int,
    pub max_tess_control_total_output_components: c_int,
    pub max_tess_evaluation_input_components: c_int,
    pub max_tess_evaluation_output_components: c_int,
    pub max_tess_evaluation_texture_image_units: c_int,
    pub max_tess_evaluation_uniform_components: c_int,
    pub max_tess_patch_components: c_int,
    pub max_patch_vertices: c_int,
    pub max_tess_gen_level: c_int,
    pub max_viewports: c_int,
    pub max_vertex_atomic_counters: c_int,
    pub max_tess_control_atomic_counters: c_int,
    pub max_tess_evaluation_atomic_counters: c_int,
    pub max_geometry_atomic_counters: c_int,
    pub max_fragment_atomic_counters: c_int,
    pub max_combined_atomic_counters: c_int,
    pub max_atomic_counter_bindings: c_int,
    pub max_vertex_atomic_counter_buffers: c_int,
    pub max_tess_control_atomic_counter_buffers: c_int,
    pub max_tess_evaluation_atomic_counter_buffers: c_int,
    pub max_geometry_atomic_counter_buffers: c_int,
    pub max_fragment_atomic_counter_buffers: c_int,
    pub max_combined_atomic_counter_buffers: c_int,
    pub max_atomic_counter_buffer_size: c_int,
    pub max_transform_feedback_buffers: c_int,
    pub max_transform_feedback_interleaved_components: c_int,
    pub max_cull_distances: c_int,
    pub max_combined_clip_and_cull_distances: c_int,
    pub max_samples: c_int,
    pub max_mesh_output_vertices_nv: c_int,
    pub max_mesh_output_primitives_nv: c_int,
    pub max_mesh_work_group_size_x_nv: c_int,
    pub max_mesh_work_group_size_y_nv: c_int,
    pub max_mesh_work_group_size_z_nv: c_int,
    pub max_task_work_group_size_x_nv: c_int,
    pub max_task_work_group_size_y_nv: c_int,
    pub max_task_work_group_size_z_nv: c_int,
    pub max_mesh_view_count_nv: c_int,
    pub max_mesh_output_vertices_ext: c_int,
    pub max_mesh_output_primitives_ext: c_int,
    pub max_mesh_work_group_size_x_ext: c_int,
    pub max_mesh_work_group_size_y_ext: c_int,
    pub max_mesh_work_group_size_z_ext: c_int,
    pub max_task_work_group_size_x_ext: c_int,
    pub max_task_work_group_size_y_ext: c_int,
    pub max_task_work_group_size_z_ext: c_int,
    pub max_mesh_view_count_ext: c_int,
    pub max_dual_source_draw_buffers_ext: c_int,

    pub limits: Limits
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct Input {
    pub language: Source,
    pub stage: Stage,
    pub client: Client,
    pub client_version: TargetClientVersion,
    pub target_language: TargetLanguage,
    pub target_language_version: TargetLanguageVersion,
    /* Shader source code */
    pub code: *const c_char,
    pub default_version: c_int,
    pub default_profile: Profile,
    pub force_default_version_and_profile: c_int,
    pub forward_compatible: c_int,
    pub messages: Messages,
    pub resource: *const Resource,
    pub callbacks: IncludeCallbacks,
    pub callbacks_ctx: *mut c_void,
}

/* Inclusion result structure allocated by C include_local/include_system callbacks */
#[repr(C)]
#[derive(Clone, Copy)]
pub struct IncludeResult {
    /* Header file name or NULL if inclusion failed */
    pub header_name: *const c_char,

    /* Header contents or NULL */
    pub header_data: *const c_char,
    pub header_length: usize
}

/* Callback for local file inclusion */
pub type GlslIncludeLocalFunc = extern "C" fn (ctx: *mut c_void, header_name: *const c_char, includer_name: *const c_char,
                                                        include_depth: usize) -> *mut IncludeResult;

/* Callback for system file inclusion */
pub type GlslIncludeSystemFunc = extern "C" fn (ctx: *mut c_void, header_name: *const c_char,
                                   includer_name: *const c_char, include_depth: usize) -> *mut IncludeResult;

/* Callback for include result destruction */
pub type GlslFreeIncludeResultFunc = extern "C" fn (ctx: *mut c_void, result: *mut IncludeResult) -> c_int;

/* Collection of callbacks for GLSL preprocessor */
#[repr(C)]
#[derive(Clone, Copy)]
pub struct IncludeCallbacks {
    pub include_system: Option<GlslIncludeSystemFunc>,
    pub include_local: Option<GlslIncludeLocalFunc>,
    pub free_include_result: Option<GlslFreeIncludeResultFunc>,
}

/* SpvOptions counterpart */
#[repr(C)]
#[derive(Clone, Copy)]
pub struct SpvOptions {
    pub generate_debug_info: bool,
    pub strip_debug_info: bool,
    pub disable_optimizer: bool,
    pub optimize_size: bool,
    pub disassemble: bool,
    pub validate: bool,
    pub emit_nonsemantic_shader_debug_info: bool,
    pub emit_nonsemantic_shader_debug_source: bool,
    pub compile_only: bool,
}

extern "C" {
    pub fn glslang_initialize_process() -> c_int;
    pub fn glslang_finalize_process();

    pub fn glslang_shader_create(input: *const Input) -> ShaderHandle;
    pub fn glslang_shader_delete(shader: ShaderHandle);
    pub fn glslang_shader_set_preamble(shader: ShaderHandle, s: *const c_char);
    pub fn glslang_shader_shift_binding(shader: ShaderHandle, res: ResourceType, base: c_uint);
    pub fn glslang_shader_shift_binding_for_set(shader: ShaderHandle, res: ResourceType, base: c_uint, set: c_uint);
    pub fn glslang_shader_set_options(shader: ShaderHandle, options: ShaderOptions); // glslang_shader_options_t
    pub fn glslang_shader_set_glsl_version(shader: ShaderHandle, version: c_int);
    pub fn glslang_shader_preprocess(shader: ShaderHandle, input: *const Input) -> c_int;
    pub fn glslang_shader_parse(shader: ShaderHandle, input: *const Input) -> c_int;
    pub fn glslang_shader_get_preprocessed_code(shader: ShaderHandle) -> *const c_char;
    pub fn glslang_shader_get_info_log(shader: ShaderHandle) -> *const c_char;
    pub fn glslang_shader_get_info_debug_log(shader: ShaderHandle) -> *const c_char;

    pub fn glslang_program_create() -> ProgramHandle;
    pub fn glslang_program_delete(program: ProgramHandle);
    pub fn glslang_program_add_shader(program: ProgramHandle, shader: ShaderHandle);
    pub fn glslang_program_link(program: ProgramHandle, messages: Messages) -> c_int; // glslang_messages_t
    pub fn glslang_program_add_source_text(program: ProgramHandle, stage: Stage, text: *const c_char, len: usize);
    pub fn glslang_program_set_source_file(program: ProgramHandle, stage: Stage, file: *const c_char);
    pub fn glslang_program_map_io(program: ProgramHandle) -> c_int;
    pub fn glslang_program_SPIRV_generate(program: ProgramHandle, stage: Stage);
    pub fn glslang_program_SPIRV_generate_with_options(program: ProgramHandle, stage: Stage, spv_options: *mut SpvOptions);
    pub fn glslang_program_SPIRV_get_size(program: ProgramHandle) -> usize;
    pub fn glslang_program_SPIRV_get(program: ProgramHandle, i: *mut c_uint);
    pub fn glslang_program_SPIRV_get_ptr(program: ProgramHandle) -> *mut c_uint;
    pub fn glslang_program_SPIRV_get_messages(program: ProgramHandle) -> *const c_char;
    pub fn glslang_program_get_info_log(program: ProgramHandle) -> *const c_char;
    pub fn glslang_program_get_info_debug_log(program: ProgramHandle) -> *const c_char;
}

/* /glslang_c_interface.h */

/* default built-in resource initialization */

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
    }
};

/* /default resources initialization */
