use argus_scripting_bind::script_bind;
use argus_util::math::Vector3f;
use crate::common::Transform2d;

#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq)]
#[script_bind]
pub enum Light2dType {
    Point = 0,
}

#[derive(Clone, Copy, Debug)]
#[script_bind]
pub struct Light2dProperties {
    /// The type of light.
    pub ty: Light2dType,
    /// The RGB color of the light as normalized float values.
    pub color: Vector3f,
    /// Whether the light can be occluded by objects.
    pub is_occludable: bool,
    /// The absolute intensity of the light between 0 and 1.
    pub intensity: f32,
    /// How sharply the light drops off. Values above 1 will result in a slower
    /// falloff close to the light and a steeper one further away from it, while
    /// values between 0 and 1 will 
    pub falloff_gradient: f32,
    /// The distance (outside the falloff buffer) over which the light intensity
    /// falls to zero.
    pub falloff_distance: f32,
    /// The distance at which the light will begin to fall off. At shorter
    /// distances the light will be at full intensity.
    pub falloff_buffer: f32,
    /// How sharply the light drops off when occluded by an object. See
    /// `falloff_gradient` for more information.
    pub shadow_falloff_gradient: f32,
    /// The distance over which the light intensity falls to zero after being
    /// occluded by an object.
    pub shadow_falloff_distance: f32,
}

impl Default for Light2dProperties {
    fn default() -> Self {
        Self {
            ty: Light2dType::Point,
            color: Default::default(),
            is_occludable: false,
            intensity: 1.0,
            falloff_gradient: 1.0,
            falloff_distance: 1.0,
            falloff_buffer: 0.0,
            shadow_falloff_gradient: 1.0,
            shadow_falloff_distance: 1.0,
        }
    }
}

pub struct RenderLight2d {
    properties: Light2dProperties,
    transform: Transform2d,
}

impl RenderLight2d {
    #[must_use]
    pub(crate) fn new(
        properties: Light2dProperties,
        transform: Transform2d
    ) -> RenderLight2d {
        Self {
            properties,
            transform,
        }
    }

    #[must_use]
    pub fn get_properties(&self) -> &Light2dProperties {
        &self.properties
    }

    #[must_use]
    pub fn get_properties_mut(&mut self) -> &mut Light2dProperties {
        &mut self.properties
    }

    pub fn set_properties(&mut self, params: Light2dProperties) {
        self.properties = params;
    }

    #[must_use]
    pub fn get_transform(&self) -> &Transform2d {
        &self.transform
    }

    pub fn set_transform(&mut self, transform: Transform2d) {
        if transform == self.transform {
            return;
        }
        self.transform = transform;
    }
}
