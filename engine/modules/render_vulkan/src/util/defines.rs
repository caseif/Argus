use ash::vk;

pub(crate) const BINDING_INDEX_VBO: u32 = 0;
pub(crate) const BINDING_INDEX_ANIM_FRAME_BUF: u32 = 1;

pub(crate) const SHADER_ATTRIB_POSITION_LEN: u32 = 2;
pub(crate) const SHADER_ATTRIB_NORMAL_LEN: u32 = 2;
pub(crate) const SHADER_ATTRIB_COLOR_LEN: u32 = 4;
pub(crate) const SHADER_ATTRIB_TEXCOORD_LEN: u32 = 2;
pub(crate) const SHADER_ATTRIB_ANIM_FRAME_LEN: u32 = 2;

pub(crate) const SHADER_ATTRIB_POSITION_FORMAT: vk::Format = vk::Format::R32G32_SFLOAT;
pub(crate) const SHADER_ATTRIB_NORMAL_FORMAT: vk::Format = vk::Format::R32G32_SFLOAT;
pub(crate) const SHADER_ATTRIB_COLOR_FORMAT: vk::Format = vk::Format::R32G32B32A32_SFLOAT;
pub(crate) const SHADER_ATTRIB_TEXCOORD_FORMAT: vk::Format = vk::Format::R32G32_SFLOAT;
pub(crate) const SHADER_ATTRIB_ANIM_FRAME_FORMAT: vk::Format = vk::Format::R32G32_SFLOAT;

pub(crate) const SHADER_OUT_COLOR_LOC: u32 = 0;
pub(crate) const SHADER_OUT_LIGHT_OPACITY_LOC: u32 = 1;

pub(crate) const FB_SHADER_VERT_PATH: &str = "argus:shader/vk/framebuffer_vert";
pub(crate) const FB_SHADER_FRAG_PATH: &str = "argus:shader/vk/framebuffer_frag";

pub(crate) const MAX_FRAMES_IN_FLIGHT: usize = 2;
