use argus_shadertools::ShaderReflectionInfo;
use crate::vk;

pub use ash::vk::DynamicState;
pub use ash::vk::PipelineBindPoint;
pub use ash::vk::PipelineColorBlendAttachmentState;
pub use ash::vk::PipelineCreateFlags;
pub use ash::vk::PipelineStageFlags;
pub use ash::vk::PrimitiveTopology;
pub use ash::vk::VertexInputAttributeDescription;
pub use ash::vk::VertexInputBindingDescription;
pub use ash::vk::VertexInputRate;

// rasterization
pub use ash::vk::CullModeFlags;
pub use ash::vk::FrontFace;
pub use ash::vk::PolygonMode;
pub use ash::vk::SampleCountFlags;
use crate::vk::Wrapper;

pub struct PipelineLayout<'ctx> {
    device: &'ctx vk::Device<'ctx>,
    underlying: ash::vk::PipelineLayout,
}

impl<'ctx> Wrapper for PipelineLayout<'ctx> {
    type Underlying = ash::vk::PipelineLayout;

    unsafe fn get_underlying(&self) -> ash::vk::PipelineLayout {
        self.underlying
    }
}

impl<'ctx> PipelineLayout<'ctx> {
    pub fn create(
        device: &'ctx vk::Device<'ctx>,
        desc_set_layouts: &[vk::DescriptorSetLayout<'ctx>],
    ) -> Result<Self, String> {
        let vk_ds_layouts = desc_set_layouts.iter()
            .map(|dsl| unsafe { dsl.get_underlying() })
            .collect::<Vec<_>>();
        let pipeline_layout_info = ash::vk::PipelineLayoutCreateInfo::default()
            .set_layouts(&vk_ds_layouts)
            .push_constant_ranges(&[]);

        let pipeline_layout = unsafe {
            device.get_underlying().create_pipeline_layout(&pipeline_layout_info, None)
                .map_err(|err| err.to_string())?
        };

        Ok(Self {
            device,
            underlying: pipeline_layout,
        })
    }

    pub fn destroy(self) {
        unsafe {
            self.device.get_underlying().destroy_pipeline_layout(self.underlying, None);
        }
    }
}

pub struct Pipeline<'ctx> {
    device: &'ctx vk::Device<'ctx>,
    underlying: ash::vk::Pipeline,
    pub layout: PipelineLayout<'ctx>,
    pub reflection: ShaderReflectionInfo,
    pub vertex_len: u32,
}

impl<'ctx> Wrapper for Pipeline<'ctx> {
    type Underlying = ash::vk::Pipeline;

    unsafe fn get_underlying(&self) -> ash::vk::Pipeline {
        self.underlying
    }
}

impl<'ctx> PartialEq for Pipeline<'ctx> {
    fn eq(&self, other: &Self) -> bool {
        self.underlying == other.underlying
    }
}

impl<'ctx> Pipeline<'ctx> {
    pub fn create_graphics_pipeline(
        device: &'ctx vk::Device<'ctx>,
        desc_set_layout: vk::DescriptorSetLayout<'ctx>,
        create_info: GraphicsPipelineCreateInfo<'_, 'ctx>,
        reflection: ShaderReflectionInfo,
        vertex_len: u32,
    ) -> Result<Self, String> {
        let pipeline_layout = PipelineLayout::create(device, &[desc_set_layout])?;
        let create_info = create_info
            .layout(&pipeline_layout);
        let prepared_create_info = create_info.prepare();

        let pipeline = unsafe {
            device.get_underlying().create_graphics_pipelines(
                ash::vk::PipelineCache::null(),
                &[prepared_create_info.create_underlying()],
                None,
            )
                .map_err(|err| err.1.to_string())?[0]
        };

        create_info.destroy();

        Ok(Self {
            device,
            underlying: pipeline,
            layout: pipeline_layout,
            reflection,
            vertex_len,
        })
    }

    pub fn destroy(
        self,
    ) {
        self.layout.destroy();

        unsafe {
            self.device.get_underlying().destroy_pipeline(self.underlying, None);
        }
    }
}

pub struct GraphicsPipelineCreateInfo<'info, 'ctx: 'info> {
    flags: vk::PipelineCreateFlags,
    stages: Option<Vec<vk::PipelineShaderStageCreateInfo<'ctx>>>,
    vertex_input_state: Option<PipelineVertexInputStateCreateInfo>,
    input_assembly_state: Option<PipelineInputAssemblyStateCreateInfo>,
    viewport_state: Option<PipelineViewportStateCreateInfo>,
    rasterization_state: Option<PipelineRasterizationStateCreateInfo>,
    multisample_state: Option<PipelineMultisampleStateCreateInfo>,
    color_blend_state: Option<PipelineColorBlendStateCreateInfo>,
    dynamic_state: Option<PipelineDynamicStateCreateInfo>,
    depth_stencil_state: Option<PipelineDepthStencilStateCreateInfo>,
    layout: Option<&'ctx PipelineLayout<'ctx>>,
    render_pass: Option<&'info vk::RenderPass<'ctx>>,
    subpass: u32,
    base_pipeline_handle: Option<&'ctx Pipeline<'ctx>>,
    base_pipeline_index: i32,
}

impl<'info, 'ctx: 'info> Default for GraphicsPipelineCreateInfo<'info, 'ctx> {
    fn default() -> Self {
        Self {
            flags: vk::PipelineCreateFlags::empty(),
            stages: None,
            vertex_input_state: None,
            input_assembly_state: None,
            viewport_state: None,
            rasterization_state: None,
            multisample_state: None,
            color_blend_state: None,
            dynamic_state: None,
            depth_stencil_state: None,
            layout: None,
            render_pass: None,
            subpass: 0,
            base_pipeline_handle: None,
            base_pipeline_index: 0,
        }
    }
}

impl<'info, 'ctx: 'info> GraphicsPipelineCreateInfo<'info, 'ctx> {
    pub fn flags(mut self, flags: vk::PipelineCreateFlags) -> Self {
        self.flags = flags;
        self
    }

    pub fn stages(mut self, stages: impl Into<Vec<vk::PipelineShaderStageCreateInfo<'ctx>>>)
        -> Self {
        self.stages = Some(stages.into());
        self
    }

    pub fn vertex_input_state(
        mut self,
        vertex_input_state: PipelineVertexInputStateCreateInfo,
    ) -> Self {
        self.vertex_input_state = Some(vertex_input_state);
        self
    }

    pub fn input_assembly_state(
        mut self,
        input_assembly_state: PipelineInputAssemblyStateCreateInfo,
    ) -> Self {
        self.input_assembly_state = Some(input_assembly_state);
        self
    }

    pub fn viewport_state(
        mut self,
        viewport_state: PipelineViewportStateCreateInfo,
    ) -> Self {
        self.viewport_state = Some(viewport_state);
        self
    }

    pub fn rasterization_state(
        mut self,
        rasterization_state: PipelineRasterizationStateCreateInfo,
    ) -> Self {
        self.rasterization_state = Some(rasterization_state);
        self
    }

    pub fn multisample_state(
        mut self,
        multisample_state: PipelineMultisampleStateCreateInfo,
    ) -> Self {
        self.multisample_state = Some(multisample_state);
        self
    }

    pub fn color_blend_state(
        mut self,
        color_blend_state: PipelineColorBlendStateCreateInfo,
    ) -> Self {
        self.color_blend_state = Some(color_blend_state);
        self
    }

    pub fn dynamic_state(mut self, dynamic_state: PipelineDynamicStateCreateInfo) -> Self {
        self.dynamic_state = Some(dynamic_state);
        self
    }

    pub fn depth_stencil_state(
        mut self,
        depth_stencil_state: PipelineDepthStencilStateCreateInfo,
    ) -> Self {
        self.depth_stencil_state = Some(depth_stencil_state);
        self
    }

    pub fn layout(mut self, layout: &'ctx PipelineLayout<'ctx>) -> Self {
        self.layout = Some(layout);
        self
    }

    pub fn render_pass(mut self, render_pass: &'info vk::RenderPass<'ctx>) -> Self {
        self.render_pass = Some(render_pass);
        self
    }

    pub fn subpass(mut self, subpass: u32) -> Self {
        self.subpass = subpass;
        self
    }

    pub fn base_pipeline_handle(mut self, base_pipeline_handle: &'ctx Pipeline<'ctx>) -> Self {
        self.base_pipeline_handle = Some(base_pipeline_handle);
        self
    }

    pub fn base_pipeline_index(mut self, base_pipeline_index: i32) -> Self {
        self.base_pipeline_index = base_pipeline_index;
        self
    }

    fn prepare(&'info self) -> PreparedGraphicsPipelineCreateInfo<'info, 'ctx> {
        PreparedGraphicsPipelineCreateInfo {
            flags: self.flags,
            stages: self.stages.as_ref().map(|v| v.iter().map(|s| s.create_underlying()).collect()),
            vertex_input_state: self.vertex_input_state.as_ref().map(|s| s.create_underlying()),
            input_assembly_state: self.input_assembly_state.as_ref().map(|s| s.create_underlying()),
            viewport_state: self.viewport_state.as_ref().map(|s| s.create_underlying()),
            rasterization_state: self.rasterization_state.as_ref().map(|s| s.create_underlying()),
            multisample_state: self.multisample_state.as_ref().map(|s| s.create_underlying()),
            color_blend_state: self.color_blend_state.as_ref().map(|s| s.create_underlying()),
            dynamic_state: self.dynamic_state.as_ref().map(|s| s.create_underlying()),
            depth_stencil_state: self.depth_stencil_state.as_ref().map(|s| s.create_underlying()),
            layout: self.layout,
            render_pass: self.render_pass,
            subpass: self.subpass,
            base_pipeline_handle: self.base_pipeline_handle,
            base_pipeline_index: self.base_pipeline_index,
        }
    }

    pub fn destroy(self) {
        if let Some(stages) = self.stages {
            for stage in stages {
                stage.destroy();
            }
        }
    }
}

struct PreparedGraphicsPipelineCreateInfo<'info, 'ctx: 'info> {
    flags: vk::PipelineCreateFlags,
    stages: Option<Vec<ash::vk::PipelineShaderStageCreateInfo<'info>>>,
    vertex_input_state: Option<ash::vk::PipelineVertexInputStateCreateInfo<'info>>,
    input_assembly_state: Option<ash::vk::PipelineInputAssemblyStateCreateInfo<'info>>,
    viewport_state: Option<ash::vk::PipelineViewportStateCreateInfo<'info>>,
    rasterization_state: Option<ash::vk::PipelineRasterizationStateCreateInfo<'info>>,
    multisample_state: Option<ash::vk::PipelineMultisampleStateCreateInfo<'info>>,
    color_blend_state: Option<ash::vk::PipelineColorBlendStateCreateInfo<'info>>,
    dynamic_state: Option<ash::vk::PipelineDynamicStateCreateInfo<'info>>,
    depth_stencil_state: Option<ash::vk::PipelineDepthStencilStateCreateInfo<'info>>,
    layout: Option<&'info vk::PipelineLayout<'ctx>>,
    render_pass: Option<&'info vk::RenderPass<'ctx>>,
    subpass: u32,
    base_pipeline_handle: Option<&'ctx vk::Pipeline<'ctx>>,
    base_pipeline_index: i32,
}

impl<'info, 'ctx: 'info> PreparedGraphicsPipelineCreateInfo<'info, 'ctx> {
    pub(crate) fn create_underlying(&self) -> ash::vk::GraphicsPipelineCreateInfo<'_> {
        let mut info = ash::vk::GraphicsPipelineCreateInfo::default()
            .flags(self.flags)
            .subpass(self.subpass)
            .base_pipeline_index(self.base_pipeline_index);

        if let Some(render_pass) = self.render_pass.as_ref() {
            info = info.render_pass(unsafe { render_pass.get_underlying() });
        }
        if let Some(base_pipeline_handle) = self.base_pipeline_handle.as_ref() {
            info = info.base_pipeline_handle(unsafe { base_pipeline_handle.get_underlying() });
        }
        if let Some(layout) = self.layout.as_ref() {
            info = info.layout(layout.underlying);
        }
        if let Some(dynamic_state) = self.dynamic_state.as_ref() {
            info = info.dynamic_state(dynamic_state);
        }

        if let Some(stages) = self.stages.as_ref() {
            info = info.stages(&stages);
        }
        if let Some(vertex_input_state) = self.vertex_input_state.as_ref() {
            info = info.vertex_input_state(&vertex_input_state);
        }
        if let Some(input_assembly_state) = self.input_assembly_state.as_ref() {
            info = info.input_assembly_state(&input_assembly_state);
        }
        if let Some(viewport_state) = self.viewport_state.as_ref() {
            info = info.viewport_state(&viewport_state);
        }
        if let Some(rasterization_state) = self.rasterization_state.as_ref() {
            info = info.rasterization_state(&rasterization_state);
        }
        if let Some(multisample_state) = self.multisample_state.as_ref() {
            info = info.multisample_state(&multisample_state);
        }
        if let Some(color_blend_state) = self.color_blend_state.as_ref() {
            info = info.color_blend_state(&color_blend_state);
        }
        if let Some(depth_stencil_state) = self.depth_stencil_state.as_ref() {
            info = info.depth_stencil_state(&depth_stencil_state);
        }

        info
    }
}

#[derive(Default)]
pub struct PipelineVertexInputStateCreateInfo {
    vertex_binding_descriptions: Vec<vk::VertexInputBindingDescription>,
    vertex_attribute_descriptions: Vec<vk::VertexInputAttributeDescription>,
}

impl PipelineVertexInputStateCreateInfo {
    pub fn vertex_binding_descriptions(
        mut self,
        descriptions: impl Into<Vec<vk::VertexInputBindingDescription>>
    ) -> Self {
        self.vertex_binding_descriptions = descriptions.into();
        self
    }
    
    pub fn vertex_attribute_descriptions(
        mut self,
        descriptions: impl Into<Vec<vk::VertexInputAttributeDescription>>
    ) -> Self {
        self.vertex_attribute_descriptions = descriptions.into();
        self
    }
    
    pub(crate) fn create_underlying(&'_ self) -> ash::vk::PipelineVertexInputStateCreateInfo<'_> {
        ash::vk::PipelineVertexInputStateCreateInfo::default()
            .vertex_binding_descriptions(&self.vertex_binding_descriptions)
            .vertex_attribute_descriptions(&self.vertex_attribute_descriptions)
    }
}

#[derive(Default)]
pub struct PipelineInputAssemblyStateCreateInfo {
    topology: vk::PrimitiveTopology,
    primitive_restart_enable: bool,
}

impl PipelineInputAssemblyStateCreateInfo {
    pub fn topology(mut self, topology: vk::PrimitiveTopology) -> Self {
        self.topology = topology;
        self
    }

    pub fn primitive_restart_enable(mut self, enable: bool) -> Self {
        self.primitive_restart_enable = enable;
        self
    }

    pub(crate) fn create_underlying(&'_ self) -> ash::vk::PipelineInputAssemblyStateCreateInfo<'_> {
        ash::vk::PipelineInputAssemblyStateCreateInfo::default()
            .topology(self.topology)
            .primitive_restart_enable(self.primitive_restart_enable)
    }
}

#[derive(Default)]
pub struct PipelineViewportStateCreateInfo {
    viewports: Vec<vk::Viewport>,
    scissors: Vec<vk::Rect2D>,
}

impl PipelineViewportStateCreateInfo {
    pub fn viewports(mut self, viewports: impl Into<Vec<vk::Viewport>>) -> Self {
        self.viewports = viewports.into();
        self
    }

    pub fn scissors(mut self, scissors: impl Into<Vec<vk::Rect2D>>) -> Self {
        self.scissors = scissors.into();
        self
    }

    pub(crate) fn create_underlying(&'_ self) -> ash::vk::PipelineViewportStateCreateInfo<'_> {
        ash::vk::PipelineViewportStateCreateInfo::default()
            .viewports(&self.viewports)
            .scissors(&self.scissors)
    }
}

#[derive(Default)]
pub struct PipelineRasterizationStateCreateInfo {
    depth_clamp_enable: bool,
    rasterizer_discard_enable: bool,
    polygon_mode: vk::PolygonMode,
    cull_mode: vk::CullModeFlags,
    front_face: vk::FrontFace,
    depth_bias_enable: bool,
    depth_bias_constant_factor: f32,
    depth_bias_clamp: f32,
    depth_bias_slope_factor: f32,
    line_width: f32,
}

impl PipelineRasterizationStateCreateInfo {
    pub fn depth_clamp_enable(mut self, enable: bool) -> Self {
        self.depth_clamp_enable = enable;
        self
    }

    pub fn rasterizer_discard_enable(mut self, enable: bool) -> Self {
        self.rasterizer_discard_enable = enable;
        self
    }

    pub fn polygon_mode(mut self, polygon_mode: vk::PolygonMode) -> Self {
        self.polygon_mode = polygon_mode;
        self
    }

    pub fn cull_mode(mut self, cull_mode: vk::CullModeFlags) -> Self {
        self.cull_mode = cull_mode;
        self
    }

    pub fn front_face(mut self, front_face: vk::FrontFace) -> Self {
        self.front_face = front_face;
        self
    }

    pub fn depth_bias_enable(mut self, enable: bool) -> Self {
        self.depth_bias_enable = enable;
        self
    }

    pub fn depth_bias_constant_factor(mut self, factor: f32) -> Self {
        self.depth_bias_constant_factor = factor;
        self
    }

    pub fn depth_bias_clamp(mut self, clamp: f32) -> Self {
        self.depth_bias_clamp = clamp;
        self
    }

    pub fn depth_bias_slope_factor(mut self, factor: f32) -> Self {
        self.depth_bias_slope_factor = factor;
        self
    }

    pub fn line_width(mut self, line_width: f32) -> Self {
        self.line_width = line_width;
        self
    }

    pub(crate) fn create_underlying(&'_ self) -> ash::vk::PipelineRasterizationStateCreateInfo<'_> {
        ash::vk::PipelineRasterizationStateCreateInfo::default()
            .depth_clamp_enable(self.depth_clamp_enable)
            .rasterizer_discard_enable(self.rasterizer_discard_enable)
            .polygon_mode(self.polygon_mode)
            .cull_mode(self.cull_mode)
            .front_face(self.front_face)
            .depth_bias_enable(self.depth_bias_enable)
            .depth_bias_constant_factor(self.depth_bias_constant_factor)
            .depth_bias_clamp(self.depth_bias_clamp)
            .depth_bias_slope_factor(self.depth_bias_slope_factor)
            .line_width(self.line_width)
    }
}

#[derive(Default)]
pub struct PipelineMultisampleStateCreateInfo {
    rasterization_samples: vk::SampleCountFlags,
    sample_shading_enable: bool,
    min_sample_shading: f32,
    sample_mask: Vec<ash::vk::SampleMask>,
    alpha_to_coverage_enable: bool,
    alpha_to_one_enable: bool,
}

impl PipelineMultisampleStateCreateInfo {
    pub fn rasterization_samples(mut self, samples: vk::SampleCountFlags) -> Self {
        self.rasterization_samples = samples;
        self
    }

    pub fn sample_shading_enable(mut self, enable: bool) -> Self {
        self.sample_shading_enable = enable;
        self
    }

    pub fn min_sample_shading(mut self, min_sample_shading: f32) -> Self {
        self.min_sample_shading = min_sample_shading;
        self
    }

    pub fn sample_mask(mut self, sample_mask: impl Into<Vec<vk::SampleMask>>) -> Self {
        self.sample_mask = sample_mask.into();
        self
    }

    pub fn alpha_to_coverage_enable(mut self, enable: bool) -> Self {
        self.alpha_to_coverage_enable = enable;
        self
    }

    pub fn alpha_to_one_enable(mut self, enable: bool) -> Self {
        self.alpha_to_one_enable = enable;
        self
    }

    pub(crate) fn create_underlying(&'_ self) -> ash::vk::PipelineMultisampleStateCreateInfo<'_> {
        ash::vk::PipelineMultisampleStateCreateInfo::default()
            .rasterization_samples(self.rasterization_samples)
            .sample_shading_enable(self.sample_shading_enable)
            .min_sample_shading(self.min_sample_shading)
            .sample_mask(&self.sample_mask)
            .alpha_to_coverage_enable(self.alpha_to_coverage_enable)
            .alpha_to_one_enable(self.alpha_to_one_enable)
    }
}

#[derive(Default)]
pub struct PipelineColorBlendStateCreateInfo {
    logic_op_enable: bool,
    logic_op: vk::LogicOp,
    attachments: Vec<vk::PipelineColorBlendAttachmentState>,
    blend_constants: [f32; 4],
}

impl PipelineColorBlendStateCreateInfo {
    pub fn logic_op_enable(mut self, enable: bool) -> Self {
        self.logic_op_enable = enable;
        self
    }

    pub fn logic_op(mut self, logic_op: vk::LogicOp) -> Self {
        self.logic_op = logic_op;
        self
    }

    pub fn attachments(
        mut self,
        attachments: impl Into<Vec<vk::PipelineColorBlendAttachmentState>>
    ) -> Self {
        self.attachments = attachments.into();
        self
    }

    pub fn blend_constants(mut self, blend_constants: [f32; 4]) -> Self {
        self.blend_constants = blend_constants;
        self
    }

    pub(crate) fn create_underlying(&'_ self) -> ash::vk::PipelineColorBlendStateCreateInfo<'_> {
        ash::vk::PipelineColorBlendStateCreateInfo::default()
            .logic_op_enable(self.logic_op_enable)
            .logic_op(self.logic_op)
            .attachments(&self.attachments)
            .blend_constants(self.blend_constants)
    }
}

#[derive(Default)]
pub struct PipelineDynamicStateCreateInfo {
    dynamic_states: Vec<vk::DynamicState>,
}

impl PipelineDynamicStateCreateInfo {
    pub fn dynamic_states(mut self, dynamic_states: impl Into<Vec<vk::DynamicState>>) -> Self {
        self.dynamic_states = dynamic_states.into();
        self
    }

    pub(crate) fn create_underlying(&'_ self) -> ash::vk::PipelineDynamicStateCreateInfo<'_> {
        ash::vk::PipelineDynamicStateCreateInfo::default()
            .dynamic_states(&self.dynamic_states)
    }
}

#[derive(Default)]
pub struct PipelineDepthStencilStateCreateInfo {
    depth_test_enable: bool,
    depth_write_enable: bool,
    depth_compare_op: vk::CompareOp,
    depth_bounds_test_enable: bool,
    stencil_test_enable: bool,
    front: ash::vk::StencilOpState,
    back: ash::vk::StencilOpState,
    min_depth_bounds: f32,
    max_depth_bounds: f32,
}

impl PipelineDepthStencilStateCreateInfo {
    pub fn depth_test_enable(mut self, enable: bool) -> Self {
        self.depth_test_enable = enable;
        self
    }

    pub fn depth_write_enable(mut self, enable: bool) -> Self {
        self.depth_write_enable = enable;
        self
    }

    pub fn depth_compare_op(mut self, depth_compare_op: vk::CompareOp) -> Self {
        self.depth_compare_op = depth_compare_op;
        self
    }

    pub fn depth_bounds_test_enable(mut self, enable: bool) -> Self {
        self.depth_bounds_test_enable = enable;
        self
    }

    pub fn stencil_test_enable(mut self, enable: bool) -> Self {
        self.stencil_test_enable = enable;
        self
    }

    pub fn front(mut self, front: ash::vk::StencilOpState) -> Self {
        self.front = front;
        self
    }

    pub fn back(mut self, back: ash::vk::StencilOpState) -> Self {
        self.back = back;
        self
    }

    pub fn min_depth_bounds(mut self, min_depth_bounds: f32) -> Self {
        self.min_depth_bounds = min_depth_bounds;
        self
    }

    pub fn max_depth_bounds(mut self, max_depth_bounds: f32) -> Self {
        self.max_depth_bounds = max_depth_bounds;
        self
    }

    pub(crate) fn create_underlying(&self) -> ash::vk::PipelineDepthStencilStateCreateInfo<'_> {
        ash::vk::PipelineDepthStencilStateCreateInfo::default()
            .depth_test_enable(self.depth_test_enable)
            .depth_write_enable(self.depth_write_enable)
            .depth_compare_op(self.depth_compare_op)
            .depth_bounds_test_enable(self.depth_bounds_test_enable)
            .stencil_test_enable(self.stencil_test_enable)
            .front(self.front)
            .back(self.back)
            .min_depth_bounds(self.min_depth_bounds)
            .max_depth_bounds(self.max_depth_bounds)
    }
}
