use ash::prelude::VkResult;
use ash::vk;
use crate::setup::device::VulkanDevice;
use crate::util::defines::{SHADER_OUT_COLOR_LOC, SHADER_OUT_LIGHT_OPACITY_LOC};

pub(crate) fn create_render_pass(
    device: &VulkanDevice,
    format: vk::Format,
    final_layout: vk::ImageLayout,
    with_supp_attachments: bool
) -> VkResult<vk::RenderPass> {
    let mut atts: Vec<vk::AttachmentDescription> = Vec::with_capacity(2);
    let mut att_refs: Vec<vk::AttachmentReference> = Vec::with_capacity(2);

    let color_att = vk::AttachmentDescription::default()
        .format(format)
        .samples(vk::SampleCountFlags::TYPE_1)
        .load_op(vk::AttachmentLoadOp::CLEAR)
        .store_op(vk::AttachmentStoreOp::STORE)
        .stencil_load_op(vk::AttachmentLoadOp::DONT_CARE)
        .stencil_store_op(vk::AttachmentStoreOp::DONT_CARE)
        .initial_layout(vk::ImageLayout::UNDEFINED)
        .final_layout(final_layout);
    atts.push(color_att);

    let color_att_ref = vk::AttachmentReference::default()
        .attachment(SHADER_OUT_COLOR_LOC)
        .layout(vk::ImageLayout::COLOR_ATTACHMENT_OPTIMAL);
    att_refs.push(color_att_ref);

    if with_supp_attachments {
        let light_opac_att = vk::AttachmentDescription::default()
            .format(vk::Format::R32_SFLOAT)
            .samples(vk::SampleCountFlags::TYPE_1)
            .load_op(vk::AttachmentLoadOp::CLEAR)
            .store_op(vk::AttachmentStoreOp::STORE)
            .stencil_load_op(vk::AttachmentLoadOp::DONT_CARE)
            .stencil_store_op(vk::AttachmentStoreOp::DONT_CARE)
            .initial_layout(vk::ImageLayout::UNDEFINED)
            .final_layout(vk::ImageLayout::SHADER_READ_ONLY_OPTIMAL);
        atts.push(light_opac_att);

        let light_opac_att_ref = vk::AttachmentReference::default()
            .attachment(SHADER_OUT_LIGHT_OPACITY_LOC)
            .layout(vk::ImageLayout::COLOR_ATTACHMENT_OPTIMAL);
        att_refs.push(light_opac_att_ref);
    }

    let subpass = vk::SubpassDescription::default()
        .pipeline_bind_point(vk::PipelineBindPoint::GRAPHICS)
        .color_attachments(att_refs.as_slice());
    let subpasses = [subpass];

    let subpass_dep = vk::SubpassDependency::default()
        .src_subpass(vk::SUBPASS_EXTERNAL)
        .dst_subpass(0)
        .src_stage_mask(vk::PipelineStageFlags::COLOR_ATTACHMENT_OUTPUT)
        .src_access_mask(vk::AccessFlags::empty())
        .dst_stage_mask(vk::PipelineStageFlags::COLOR_ATTACHMENT_OUTPUT)
        .dst_access_mask(vk::AccessFlags::empty());
    let subpass_deps = [subpass_dep];

    let render_pass_info = vk::RenderPassCreateInfo::default()
        .attachments(atts.as_slice())
        .subpasses(&subpasses)
        .dependencies(&subpass_deps);

    let render_pass = unsafe { device.logical_device.create_render_pass(&render_pass_info, None)? };

    Ok(render_pass)
}

pub(crate) fn destroy_render_pass(device: &VulkanDevice, render_pass: vk::RenderPass) {
    unsafe { device.logical_device.destroy_render_pass(render_pass, None) };
}
