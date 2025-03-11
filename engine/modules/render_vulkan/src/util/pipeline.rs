use crate::renderer::{destroy_shaders, prepare_shaders};
use crate::setup::device::VulkanDevice;
use crate::util::defines::*;
use crate::util::{create_descriptor_set_layout, destroy_descriptor_set_layout};
use argus_render::common::{Material, ShaderReflectionInfo};
use argus_render::constants::*;
use argus_util::math::Vector2u;
use ash::vk;

pub(crate) struct PipelineInfo {
    handle: vk::Pipeline,
    pub(crate) layout: vk::PipelineLayout,
    pub(crate) reflection: ShaderReflectionInfo,
    pub(crate) vertex_len: u32,

    ds_layout: vk::DescriptorSetLayout,
    desc_sets: Option<Vec<vk::DescriptorSet>>,
}

impl PipelineInfo {
    pub(crate) unsafe fn get_handle(&self) -> vk::Pipeline {
        self.handle
    }
}

fn push_attr(
    attr_descs: &mut Vec<vk::VertexInputAttributeDescription>,
    binding: u32,
    location: u32,
    format: vk::Format,
    components: u32,
    cur_offset: u32,
) -> u32 {
    let attr_desc = vk::VertexInputAttributeDescription::default()
        .binding(binding)
        .location(location)
        .format(format)
        .offset(cur_offset);
    attr_descs.push(attr_desc);
    cur_offset + components * size_of::<f32>() as u32
}

pub(crate) fn create_pipeline_for_material(
    device: &VulkanDevice,
    material: &Material,
    viewport_size: &Vector2u,
    render_pass: vk::RenderPass,
) -> Result<PipelineInfo, String> {
    create_pipeline_for_shaders(device, material.get_shader_uids(), viewport_size, render_pass)
}

pub(crate) fn create_pipeline_for_shaders(
    device: &VulkanDevice,
    shader_uids: &[impl AsRef<str>],
    viewport_size: &Vector2u,
    render_pass: vk::RenderPass,
) -> Result<PipelineInfo, String> {
    let prepared_shaders = prepare_shaders(device, shader_uids)?;
    let shader_refl = prepared_shaders.reflection.clone();

    let dyn_states = [vk::DynamicState::VIEWPORT, vk::DynamicState::SCISSOR];

    let dyn_state_info = vk::PipelineDynamicStateCreateInfo::default()
        .dynamic_states(&dyn_states);

    let mut attr_descs: Vec<vk::VertexInputAttributeDescription> = Vec::new();
    let mut offset = 0;
    shader_refl.get_attr_loc_and_then(SHADER_ATTRIB_POSITION, |loc| {
        offset = push_attr(&mut attr_descs, BINDING_INDEX_VBO, loc, SHADER_ATTRIB_POSITION_FORMAT,
                SHADER_ATTRIB_POSITION_LEN, offset);
    });
    shader_refl.get_attr_loc_and_then(SHADER_ATTRIB_NORMAL, |loc| {
        offset = push_attr(&mut attr_descs, BINDING_INDEX_VBO, loc, SHADER_ATTRIB_NORMAL_FORMAT,
                SHADER_ATTRIB_NORMAL_LEN, offset);
    });
    shader_refl.get_attr_loc_and_then(SHADER_ATTRIB_COLOR, |loc| {
        offset = push_attr(&mut attr_descs, BINDING_INDEX_VBO, loc, SHADER_ATTRIB_COLOR_FORMAT,
                SHADER_ATTRIB_COLOR_LEN, offset);
    });
    shader_refl.get_attr_loc_and_then(SHADER_ATTRIB_TEXCOORD, |loc| {
        offset = push_attr(&mut attr_descs, BINDING_INDEX_VBO, loc, SHADER_ATTRIB_TEXCOORD_FORMAT,
                SHADER_ATTRIB_TEXCOORD_LEN, offset);
    });

    let mut binding_descs: Vec<vk::VertexInputBindingDescription> = Vec::new();
    let vbo_desc = vk::VertexInputBindingDescription::default()
        .binding(BINDING_INDEX_VBO)
        .stride(offset)
        .input_rate(vk::VertexInputRate::VERTEX);
    binding_descs.push(vbo_desc);

    if let Some(anim_frame_loc) = shader_refl.get_attr_loc(SHADER_ATTRIB_ANIM_FRAME) {
        let af_offset = 0;
        push_attr(&mut attr_descs, BINDING_INDEX_ANIM_FRAME_BUF, anim_frame_loc,
                SHADER_ATTRIB_ANIM_FRAME_FORMAT, SHADER_ATTRIB_ANIM_FRAME_LEN, af_offset);

        let anim_buf_desc = vk::VertexInputBindingDescription::default()
            .binding(BINDING_INDEX_ANIM_FRAME_BUF)
            .stride(af_offset)
            .input_rate(vk::VertexInputRate::VERTEX);

        binding_descs.push(anim_buf_desc);
    }

    let vert_in_state_info = vk::PipelineVertexInputStateCreateInfo::default()
        .vertex_binding_descriptions(&binding_descs)
        .vertex_attribute_descriptions(&attr_descs);

    let in_assembly_info = vk::PipelineInputAssemblyStateCreateInfo::default()
        .topology(vk::PrimitiveTopology::TRIANGLE_LIST)
        .primitive_restart_enable(false);

    let viewport = vk::Viewport {
        x: 0.0,
        y: 0.0,
        width: viewport_size.x as f32,
        height: viewport_size.y as f32,
        min_depth: 0.0,
        max_depth: 1.0,
    };
    let viewports = [viewport];

    let scissor = vk::Rect2D {
        offset: vk::Offset2D { x: 0, y: 0 },
        extent: vk::Extent2D { width: viewport_size.x, height: viewport_size.y },
    };
    let scissors = [scissor];

    let viewport_info = vk::PipelineViewportStateCreateInfo::default()
        .viewports(&viewports)
        .scissors(&scissors);

    let raster_info = vk::PipelineRasterizationStateCreateInfo::default()
        .depth_clamp_enable(false)
        .rasterizer_discard_enable(false)
        .polygon_mode(vk::PolygonMode::FILL)
        .line_width(1.0)
        .cull_mode(vk::CullModeFlags::NONE)
        .front_face(vk::FrontFace::COUNTER_CLOCKWISE)
        .depth_bias_enable(false)
        .depth_bias_constant_factor(0.0)
        .depth_bias_clamp(0.0)
        .depth_bias_slope_factor(0.0);

    let multisample_info = vk::PipelineMultisampleStateCreateInfo::default()
        .sample_shading_enable(false)
        .rasterization_samples(vk::SampleCountFlags::TYPE_1)
        .min_sample_shading(1.0)
        .sample_mask(&[])
        .alpha_to_coverage_enable(false)
        .alpha_to_one_enable(false);

    let mut atts: Vec<vk::PipelineColorBlendAttachmentState> = Vec::with_capacity(2);

    let color_blend_att = vk::PipelineColorBlendAttachmentState::default()
        .color_write_mask(
            vk::ColorComponentFlags::R |
            vk::ColorComponentFlags::G |
            vk::ColorComponentFlags::B |
            vk::ColorComponentFlags::A
        )
        .blend_enable(true)
        .src_color_blend_factor(vk::BlendFactor::SRC_ALPHA)
        .dst_color_blend_factor(vk::BlendFactor::ONE_MINUS_SRC_ALPHA)
        .color_blend_op(vk::BlendOp::ADD)
        .src_alpha_blend_factor(vk::BlendFactor::ONE)
        .dst_alpha_blend_factor(vk::BlendFactor::ONE)
        .alpha_blend_op(vk::BlendOp::ADD);

    atts.push(color_blend_att);

    if shader_refl.has_output(SHADER_OUT_LIGHT_OPACITY) {
        let light_opac_blend_att = vk::PipelineColorBlendAttachmentState::default()
            .color_write_mask(vk::ColorComponentFlags::R)
            .blend_enable(true)
            .src_color_blend_factor(vk::BlendFactor::ONE)
            .dst_color_blend_factor(vk::BlendFactor::DST_ALPHA)
            .color_blend_op(vk::BlendOp::ADD)
            .src_alpha_blend_factor(vk::BlendFactor::ONE)
            .dst_alpha_blend_factor(vk::BlendFactor::ONE)
            .alpha_blend_op(vk::BlendOp::ADD);
        atts.push(light_opac_blend_att);
    }

    let color_blend_info = vk::PipelineColorBlendStateCreateInfo::default()
        .logic_op_enable(false)
        .logic_op(vk::LogicOp::COPY)
        .attachments(&atts)
        .blend_constants([0.0, 0.0, 0.0, 0.0]);

    let ds_layout = create_descriptor_set_layout(device, &shader_refl)?;
    let ds_layouts = [ds_layout];

    let pipeline_layout_info = vk::PipelineLayoutCreateInfo::default()
        .set_layouts(&ds_layouts)
        .push_constant_ranges(&[]);

    let pipeline_layout = unsafe {
        device.logical_device.create_pipeline_layout(&pipeline_layout_info, None)
            .map_err(|err| err.to_string())?
    };

    let Some(out_color_loc) = shader_refl.get_output_loc(SHADER_OUT_COLOR) else {
        return Err(format!("Required shader output {SHADER_OUT_COLOR} is missing").to_owned());
    };
    if out_color_loc != SHADER_OUT_COLOR_LOC {
        return Err(
            format!("Required shader output {SHADER_OUT_COLOR} must have location 0").to_owned()
        );
    }

    let depth_info = vk::PipelineDepthStencilStateCreateInfo::default()
        .depth_test_enable(true)
        .depth_write_enable(false)
        .depth_compare_op(vk::CompareOp::ALWAYS)
        .depth_bounds_test_enable(false)
        .stencil_test_enable(false);

    let pipeline_info = vk::GraphicsPipelineCreateInfo::default()
        .stages(&prepared_shaders.stages)
        .vertex_input_state(&vert_in_state_info)
        .input_assembly_state(&in_assembly_info)
        .viewport_state(&viewport_info)
        .rasterization_state(&raster_info)
        .multisample_state(&multisample_info)
        .color_blend_state(&color_blend_info)
        .dynamic_state(&dyn_state_info)
        .depth_stencil_state(&depth_info)
        .layout(pipeline_layout)
        .render_pass(render_pass);

    let pipeline = unsafe {
        device.logical_device.create_graphics_pipelines(
            vk::PipelineCache::null(),
            &[pipeline_info],
            None,
        )
            .map_err(|err| err.1.to_string())?[0]
    };

    destroy_shaders(device, prepared_shaders);

    Ok(PipelineInfo {
        handle: pipeline,
        layout: pipeline_layout,
        ds_layout,
        reflection: shader_refl,
        vertex_len: offset,
        desc_sets: None,
    })
}

pub(crate) fn destroy_pipeline(
    device: &VulkanDevice,
    pipeline: PipelineInfo,
) {
    destroy_descriptor_set_layout(device, pipeline.ds_layout);

    unsafe {
        device.logical_device.destroy_pipeline(pipeline.handle, None);
        device.logical_device.destroy_pipeline_layout(pipeline.layout, None);
    }
}
