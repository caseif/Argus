use crate::vk;

pub use ash::vk::ClearValue;
pub use ash::vk::ClearColorValue;
pub use ash::vk::RenderPassCreateInfo;
pub use ash::vk::SubpassContents;
use crate::vk::Wrapper;

pub struct RenderPass<'ctx> {
    device: &'ctx vk::Device<'ctx>,
    underlying: ash::vk::RenderPass,
}

impl<'ctx> Wrapper for RenderPass<'ctx> {
    type Underlying = ash::vk::RenderPass;
    
    unsafe fn get_underlying(&self) -> ash::vk::RenderPass {
        self.underlying
    }
}

#[derive(Default)]
pub struct RenderPassBeginInfo<'ctx> {
    framebuffer: Option<&'ctx vk::Framebuffer<'ctx>>,
    clear_values: Option<Vec<vk::ClearValue>>,
    render_pass: Option<&'ctx RenderPass<'ctx>>,
    render_area: Option<vk::Rect2D>,
}

impl<'ctx> RenderPassBeginInfo<'ctx> {
    pub fn framebuffer(mut self, framebuffer: &'ctx vk::Framebuffer<'ctx>) -> Self {
        self.framebuffer = Some(framebuffer);
        self
    }

    pub fn clear_values(mut self, clear_values: impl Into<Vec<vk::ClearValue>>) -> Self {
        self.clear_values= Some(clear_values.into());
        self
    }

    pub fn render_pass(mut self, render_pass: &'ctx RenderPass<'ctx>) -> Self {
        self.render_pass = Some(render_pass);
        self
    }

    pub fn render_area(mut self, render_area: vk::Rect2D) -> Self {
        self.render_area = Some(render_area);
        self
    }

    pub(crate) fn create_underlying(&'_ self) -> ash::vk::RenderPassBeginInfo<'_> {
        ash::vk::RenderPassBeginInfo::default()
            .framebuffer(unsafe { self.framebuffer.unwrap().get_underlying() })
            .clear_values(self.clear_values.as_ref().unwrap())
            .render_pass(unsafe { self.render_pass.unwrap().get_underlying() })
            .render_area(self.render_area.unwrap())
    }
}

impl<'ctx> RenderPass<'ctx> {
    pub fn create(
        device: &'ctx vk::Device<'ctx>,
        create_info: &vk::RenderPassCreateInfo,
    ) -> Result<Self, String> {
        let pass = unsafe {
            device.get_underlying().create_render_pass(create_info, None)
                .map_err(|err| err.to_string())?
        };
        Ok(Self {
            device,
            underlying: pass,
        })
    }
    
    pub fn create_basic(
        device: &'ctx vk::Device<'ctx>,
        format: vk::Format,
        final_layout: vk::ImageLayout,
        color_attachment_location: u32,
        light_opacity_attachment_location: Option<u32>,
    ) -> Result<RenderPass<'ctx>, String> {
        let mut atts: Vec<ash::vk::AttachmentDescription> = Vec::with_capacity(2);
        let mut att_refs: Vec<ash::vk::AttachmentReference> = Vec::with_capacity(2);

        let color_att = ash::vk::AttachmentDescription::default()
            .format(format)
            .samples(ash::vk::SampleCountFlags::TYPE_1)
            .load_op(ash::vk::AttachmentLoadOp::CLEAR)
            .store_op(ash::vk::AttachmentStoreOp::STORE)
            .stencil_load_op(ash::vk::AttachmentLoadOp::DONT_CARE)
            .stencil_store_op(ash::vk::AttachmentStoreOp::DONT_CARE)
            .initial_layout(ash::vk::ImageLayout::UNDEFINED)
            .final_layout(final_layout);
        atts.push(color_att);

        let color_att_ref = ash::vk::AttachmentReference::default()
            .attachment(color_attachment_location)
            .layout(ash::vk::ImageLayout::COLOR_ATTACHMENT_OPTIMAL);
        att_refs.push(color_att_ref);

        if let Some(light_opac_att_loc) = light_opacity_attachment_location {
            let light_opac_att = ash::vk::AttachmentDescription::default()
                .format(ash::vk::Format::R32_SFLOAT)
                .samples(ash::vk::SampleCountFlags::TYPE_1)
                .load_op(ash::vk::AttachmentLoadOp::CLEAR)
                .store_op(ash::vk::AttachmentStoreOp::STORE)
                .stencil_load_op(ash::vk::AttachmentLoadOp::DONT_CARE)
                .stencil_store_op(ash::vk::AttachmentStoreOp::DONT_CARE)
                .initial_layout(ash::vk::ImageLayout::UNDEFINED)
                .final_layout(ash::vk::ImageLayout::SHADER_READ_ONLY_OPTIMAL);
            atts.push(light_opac_att);

            let light_opac_att_ref = ash::vk::AttachmentReference::default()
                .attachment(light_opac_att_loc)
                .layout(ash::vk::ImageLayout::COLOR_ATTACHMENT_OPTIMAL);
            att_refs.push(light_opac_att_ref);
        }

        let subpass = ash::vk::SubpassDescription::default()
            .pipeline_bind_point(ash::vk::PipelineBindPoint::GRAPHICS)
            .color_attachments(att_refs.as_slice());
        let subpasses = [subpass];

        let subpass_dep = ash::vk::SubpassDependency::default()
            .src_subpass(ash::vk::SUBPASS_EXTERNAL)
            .dst_subpass(0)
            .src_stage_mask(ash::vk::PipelineStageFlags::COLOR_ATTACHMENT_OUTPUT)
            .src_access_mask(ash::vk::AccessFlags::empty())
            .dst_stage_mask(ash::vk::PipelineStageFlags::COLOR_ATTACHMENT_OUTPUT)
            .dst_access_mask(ash::vk::AccessFlags::empty());
        let subpass_deps = [subpass_dep];

        let render_pass_info = ash::vk::RenderPassCreateInfo::default()
            .attachments(atts.as_slice())
            .subpasses(&subpasses)
            .dependencies(&subpass_deps);

        let render_pass = RenderPass::create(device, &render_pass_info)?;

        Ok(render_pass)
    }

    pub fn destroy(self) {
        unsafe { self.device.get_underlying().destroy_render_pass(self.underlying, None) };
    }
}
