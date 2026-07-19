mod defines;
mod buffer;
mod command;
mod debug;
mod descriptor;
mod device;
mod enums;
mod format;
mod framebuffer;
mod image;
mod instance;
mod memory;
mod pipeline;
mod queues;
mod render_pass;
mod sampler;
mod shader;
mod surface;
mod swapchain;
mod sync;
mod texture;
mod types;

pub use buffer::*;
pub use command::*;
pub use debug::*;
pub use defines::*;
pub use descriptor::*;
pub use device::*;
pub use enums::*;
pub use format::*;
pub use framebuffer::*;
pub use image::*;
pub use instance::*;
pub use memory::*;
pub use pipeline::*;
pub use queues::*;
pub use render_pass::*;
pub use sampler::*;
pub use shader::*;
pub use surface::*;
pub use swapchain::*;
pub use sync::*;
pub use texture::*;
pub use types::*;

use std::ops::Deref;

pub trait Wrapper: Sized {
    type Underlying;

    /// Gets the underlying Vulkan handle of the object.
    ///
    /// # Safety
    /// The returned handle must not outlive `self`.
    unsafe fn get_underlying(&self) -> Self::Underlying;
}

impl<T: Wrapper> Wrapper for Option<T> {
    type Underlying = Option<T::Underlying>;

    /// Gets the underlying Vulkan handle of the object if the Option contains a
    /// value; otherwise returns [None].
    ///
    /// # Safety
    /// The returned handle must not outlive `self`.
    unsafe fn get_underlying(&self) -> Self::Underlying {
        self.as_ref().map(|t| unsafe { t.get_underlying() })
    }
}

pub trait WrapperIterator: IntoIterator + Sized
where
    Self::Item: Deref,
    <Self::Item as Deref>::Target: Wrapper
{
    unsafe fn get_underlying(self) -> Vec<<<Self::Item as Deref>::Target as Wrapper>::Underlying> {
        self.into_iter().map(|w| unsafe { w.get_underlying() }).collect::<Vec<_>>()
    }
}

impl<T: IntoIterator> WrapperIterator for T
where
    T::Item: Deref,
    <T::Item as Deref>::Target: Wrapper,
{
}
