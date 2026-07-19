use itertools::Either;
use argus_util::math::Vector2u;
use crate::vk;
use crate::vk::Wrapper;

pub struct Framebuffer<'ctx> {
    pub device: &'ctx vk::Device<'ctx>,
    underlying: ash::vk::Framebuffer,
    pub images_or_views: Either<Vec<vk::Image<'ctx>>, Vec<vk::ImageView<'ctx>>>,
    pub sampler: Option<vk::Sampler<'ctx>>,
}

impl<'ctx> Wrapper for Framebuffer<'ctx> {
    type Underlying = ash::vk::Framebuffer;

    unsafe fn get_underlying(&self) -> ash::vk::Framebuffer {
        self.underlying
    }
}

impl<'ctx> Framebuffer<'ctx> {
    fn create(
        device: &'ctx vk::Device<'ctx>,
        render_pass: &vk::RenderPass<'ctx>,
        images_or_views: Either<Vec<vk::Image<'ctx>>, Vec<vk::ImageView<'ctx>>>,
        sampler: Option<vk::Sampler<'ctx>>,
    ) -> Result<Self, String> {
        let (size, vk_image_views) = match &images_or_views {
            Either::Left(images) => {
                assert!(!images.is_empty());

                let size = *images[0].get_size();
                for i in 0..images.len() {
                    assert!(images[i].get_view().is_some());
                    assert_eq!(images[i].get_size(), &size);
                }

                let vk_views = images
                    .iter()
                    .map(|img| unsafe { img.get_view().unwrap().get_underlying() })
                    .collect::<Vec<_>>();

                (size, vk_views)
            }
            Either::Right(views) => {
                assert!(!views.is_empty());

                let size = *views[0].get_size();
                for i in 0..views.len() {
                    assert_eq!(views[i].get_size(), &size);
                }

                (
                    size,
                    unsafe { views.iter().map(|v| v.get_underlying()).collect() },
                )
            }
        };
        let fb_info = ash::vk::FramebufferCreateInfo::default()
            .render_pass(unsafe { render_pass.get_underlying() })
            .attachments(&vk_image_views)
            .width(size.x)
            .height(size.y)
            .layers(1);

        let fb = unsafe {
            device.get_underlying().create_framebuffer(&fb_info, None)
                .map_err(|err| err.to_string())?
        };
        Ok(Self {
            device,
            underlying: fb,
            images_or_views,
            sampler,
        })
    }

    pub fn create_from_images(
        device: &'ctx vk::Device<'ctx>,
        render_pass: &vk::RenderPass<'ctx>,
        images: Vec<vk::Image<'ctx>>,
        sampler: Option<vk::Sampler<'ctx>>,
    ) -> Result<Self, String> {
        Self::create(device, render_pass, Either::Left(images), sampler)
    }

    pub(crate) unsafe fn create_from_swapchain_images(
        device: &'ctx vk::Device<'ctx>,
        render_pass: &vk::RenderPass<'ctx>,
        swapchain: ash::vk::SwapchainKHR,
        size: Vector2u,
        format: ash::vk::SurfaceFormatKHR,
    ) -> Result<Vec<Self>, String> {
        let sc_images = unsafe {
            device.khr_swapchain().get_swapchain_images(swapchain)
                .map_err(|err| format!("Failed to get swapchain images: {}", err))?
        };

        let mut framebuffers: Vec<Framebuffer> = Vec::new();

        for sc_image in &sc_images {
            let view = unsafe {
                vk::ImageView::create_from_vk_image(
                    device,
                    *sc_image,
                    size,
                    format.format,
                    vk::ImageAspectFlags::COLOR,
                )?
            };

            let framebuffer = Framebuffer::create(
                device,
                render_pass,
                Either::Right(vec![view]),
                None,
            )?;
            framebuffers.push(framebuffer);
        }

        Ok(framebuffers)
    }

    pub fn destroy(self) {
        if let Some(sampler) = self.sampler {
            sampler.destroy();
        }
        match self.images_or_views {
            Either::Left(images) => {
                for image in images {
                    image.destroy();
                }
            }
            Either::Right(views) => {
                for view in views {
                    view.destroy();
                }
            }
        }
        unsafe { self.device.get_underlying().destroy_framebuffer(self.underlying, None); }
    }

    pub fn get_images(&self) -> Option<&[vk::Image<'_>]> {
        match &self.images_or_views {
            Either::Left(images) => Some(&images),
            _ => None,
        }
    }

    #[allow(dead_code)]
    pub fn get_views(&self) -> Vec<&vk::ImageView<'_>> {
        match &self.images_or_views {
            Either::Left(images) => {
                images.iter().map(|img| img.get_view().unwrap()).collect()
            }
            Either::Right(views) => {
                views.iter().collect()
            }
        }
    }

    pub fn get_sampler(&self) -> Option<&vk::Sampler<'_>> {
        self.sampler.as_ref()
    }
}
