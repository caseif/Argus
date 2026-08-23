use crate::aglet::*;
use strum_macros::EnumIter;

pub struct GlSupport {
}

#[derive(Clone, Copy, Debug, EnumIter, PartialEq)]
pub enum GlExt {
    // GL 4.0
    DrawBuffersBlend,
    // ARB_draw_buffers_blend funcs have ARB suffix
    DrawBuffersBlendARB,
    // GL 4.2
    TextureStorage,
    ShaderImageLoadStore,
    // GL 4.3
    ClearBufferObject,
    ExplicitUniformLocation,
    VertexAttribBinding,
    ComputeShader,
    Debug,
    DebugOutputARB,
    // GL 4.4
    BufferStorage,
    // GL 4.5
    DirectStateAccess,
    // GL 4.6
    Spirv,
}

impl GlExt {
    pub fn name(&self) -> &'static str {
        match self {
            GlExt::DrawBuffersBlend => "ARB_draw_buffers_blend (core)",
            GlExt::DrawBuffersBlendARB => "ARB_draw_buffers_blend (extension)",
            GlExt::TextureStorage => "ARB_texture_storage",
            GlExt::ShaderImageLoadStore => "ARB_shader_image_load_store",
            GlExt::ClearBufferObject => "ARB_clear_buffer_object",
            GlExt::ExplicitUniformLocation => "ARB_explicit_uniform_location",
            GlExt::VertexAttribBinding => "ARB_vertex_attrib_binding",
            GlExt::ComputeShader => "ARB_compute_shader",
            GlExt::Debug => "KHR_debug",
            GlExt::DebugOutputARB => "ARB_debug_output",
            GlExt::BufferStorage => "ARB_buffer_storage",
            GlExt::DirectStateAccess => "ARB_direct_state_access",
            GlExt::Spirv => "ARB_gl_spirv",
        }
    }
}

impl GlSupport {
    pub fn have(extension: GlExt) -> bool {
        let have_min_gl_version = match extension {
            // GL 4.0
            GlExt::DrawBuffersBlend =>
                aglet_have_gl_version_4_0(),
            // GL 4.2
            GlExt::TextureStorage |
            GlExt::ShaderImageLoadStore =>
                aglet_have_gl_version_4_2(),
            // GL 4.3
            GlExt::ClearBufferObject |
            GlExt::ExplicitUniformLocation |
            GlExt::VertexAttribBinding |
            GlExt::ComputeShader |
            GlExt::Debug |
            GlExt::DebugOutputARB =>
                aglet_have_gl_version_4_3(),
            // GL 4.4
            GlExt::BufferStorage =>
                aglet_have_gl_version_4_4(),
            // GL 4.5
            GlExt::DirectStateAccess =>
                aglet_have_gl_version_4_5(),
            // GL 4.6
            GlExt::Spirv =>
                aglet_have_gl_version_4_6(),
            // Non-core
            GlExt::DrawBuffersBlendARB =>
                false,
        };

        if have_min_gl_version {
            return true;
        }

        match extension {
            GlExt::DrawBuffersBlendARB =>
                aglet_have_gl_arb_draw_buffers_blend(),
            GlExt::TextureStorage =>
                aglet_have_gl_arb_texture_storage(),
            GlExt::ShaderImageLoadStore =>
                aglet_have_gl_arb_shader_image_load_store(),
            GlExt::ClearBufferObject =>
                aglet_have_gl_arb_clear_buffer_object(),
            GlExt::ExplicitUniformLocation =>
                aglet_have_gl_arb_explicit_uniform_location(),
            GlExt::VertexAttribBinding =>
                aglet_have_gl_arb_vertex_attrib_binding(),
            GlExt::ComputeShader =>
                aglet_have_gl_arb_compute_shader(),
            GlExt::Debug =>
                aglet_have_gl_khr_debug(),
            GlExt::DebugOutputARB =>
                aglet_have_gl_arb_debug_output(),
            GlExt::BufferStorage =>
                aglet_have_gl_arb_buffer_storage(),
            // need ARB_ES2_compatibility for glShaderBinary
            GlExt::Spirv =>
                aglet_have_gl_arb_es2_compatibility() && aglet_have_gl_arb_gl_spirv(),
            GlExt::DirectStateAccess =>
                aglet_have_gl_arb_direct_state_access(),
            // Core-only, ARB version has suffix
            GlExt::DrawBuffersBlend => false,
        }
    }
}
