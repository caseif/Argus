use lowlevel_rustabi::argus::lowlevel::Vector3f;
use crate::common::Transform2d;

#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq)]
pub enum Light2dType {
    Point = 0,
}

#[derive(Clone, Copy, Debug)]
pub struct Light2dParameters {
    /// The absolute intensity of the light between 0 and 1.
    pub intensity: f32,
    /// Higher values will result in a steeper falloff gradient.
    pub falloff_gradient: u32,
    /// Higher values will increase the distance over which the light falls off.
    pub falloff_multiplier: f32,
    /// Higher values will increase the distance before the light starts to fall
    /// off.
    pub falloff_buffer: f32,
    /// Higher values will result in a steeper falloff gradient in shadows.
    pub shadow_falloff_gradient: u32,
    /// Higher values will increase the distance over which light falls off in a
    /// shadow.
    pub shadow_falloff_multiplier: f32,
}

pub struct Light2d {
    ty: Light2dType,
    is_occludable: bool,
    color: Vector3f,
    params: Light2dParameters,
    transform: Transform2d,
    version: u16,
}

impl Light2d {
    #[must_use]
    pub(crate) fn new (
        ty: Light2dType,
        is_occludable: bool,
        color: Vector3f,
        params: Light2dParameters,
        transform: Transform2d
    ) -> Light2d {
        Self {
            ty,
            is_occludable,
            color,
            params,
            transform,
            version: 0,
        }
    }

    #[must_use]
    pub fn get_type(&self) -> Light2dType {
        self.ty
    }

    #[must_use]
    pub fn is_occludable(&self) -> bool {
        self.is_occludable
    }

    #[must_use]
    pub fn get_color(&self) -> &Vector3f {
        &self.color
    }

    pub fn set_color(&mut self, color: Vector3f) {
        self.color = color;
    }

    #[must_use]
    pub fn get_parameters(&self) -> &Light2dParameters {
        &self.params
    }

    pub fn set_parameters(&mut self, params: Light2dParameters) {
        self.params = params;
    }

    #[must_use]
    pub fn get_transform(&self) -> &Transform2d {
        &self.transform
    }

    pub fn set_transform(&mut self, transform: Transform2d) {
        self.transform = transform;
    }
}
