use ash::vk;
use argus_util::math::Vector2u;
use crate::setup::device::VulkanDevice;
use crate::util::VulkanImage;

pub(crate) struct FramebufferGrouping {
    pub(crate) handle: vk::Framebuffer,
    pub(crate) images: Vec<VulkanImage>,
    pub(crate) sampler: Option<vk::Sampler>,
}

pub(crate) fn create_framebuffer_from_views(
    device: &VulkanDevice,
    render_pass: vk::RenderPass,
    image_views: &[vk::ImageView],
    size: Vector2u,
) -> Result<vk::Framebuffer, String> {
    assert!(!image_views.is_empty());

    let fb_info = vk::FramebufferCreateInfo::default()
        .render_pass(render_pass)
        .attachments(image_views)
        .width(size.x)
        .height(size.y)
        .layers(1);

    let fb = unsafe {
        device.logical_device.create_framebuffer(&fb_info, None)
            .map_err(|err| err.to_string())?
    };
    Ok(fb)
}

pub(crate) fn create_framebuffer_for_swapchain_images(
    device: &VulkanDevice,
    render_pass: vk::RenderPass,
    images: &[VulkanImage],
) -> Result<vk::Framebuffer, String> {
    assert!(!images.is_empty());

    unsafe {
        let image_views: Vec<_> = images.iter().map(|img| img.get_vk_image_view()).collect();
        create_framebuffer_from_views(
            device,
            render_pass,
            &image_views,
            *images.first().unwrap().get_size(),
        )
    }
}

pub(crate) fn destroy_framebuffer(device: &VulkanDevice, framebuffer: vk::Framebuffer) {
    unsafe {
        device.logical_device.destroy_framebuffer(framebuffer, None);
    }
}
