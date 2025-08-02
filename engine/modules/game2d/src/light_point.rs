use argus_render::common::Transform2d;
use argus_render::twod::{Light2dProperties, Light2dType};
use argus_scripting_bind::script_bind;
use argus_util::dirtiable::Dirtiable;
use argus_util::math::{Vector2f, Vector3f};
use argus_util::pool::Handle;

macro_rules! prop_getter_setter {
    ($name:ident, $ty:ty, $getter_name:ident, $setter_name:ident) => {
        #[script_bind]
        impl PointLight {
            #[script_bind]
            pub fn $getter_name(&self) -> $ty {
                self.properties.peek().value.$name
            }
            #[script_bind]
            pub fn $setter_name(&mut self, $name: $ty) {
                self.properties.update_in_place(|props| props.$name = $name);
            }
        }
    };
}

#[script_bind]
pub struct PointLight {
    pub(crate) properties: Dirtiable<Light2dProperties>,
    pub(crate) transform: Dirtiable<Transform2d>,
    pub(crate) render_light: Option<Handle>,
}

impl Clone for PointLight {
    fn clone(&self) -> Self {
        Self {
            properties: Dirtiable::new(self.properties.peek().value),
            transform: Dirtiable::new(self.transform.peek().value.clone()),
            render_light: self.render_light,
        }
    }
}

#[script_bind]
impl PointLight {
    pub fn new(position: Vector2f) -> Self {
        Self {
            properties: Dirtiable::new(Light2dProperties::default()),
            transform: Dirtiable::new(Transform2d::new(position, Vector2f::new(1.0, 1.0), 0.0)),
            render_light: None,
        }
    }

    #[script_bind]
    pub fn set_properties(&mut self, props: Light2dProperties) {
        self.properties.set(props);
    }

    #[script_bind]
    pub fn set_transform(&mut self, transform: Transform2d) {
        self.transform.set(transform);
    }
}

prop_getter_setter!(color, Vector3f, get_color, set_color);
prop_getter_setter!(is_occludable, bool, is_occludable, set_occludable);
prop_getter_setter!(intensity, f32, get_intensity, set_intensity);
prop_getter_setter!(falloff_gradient, f32, get_falloff_gradient, set_falloff_gradient);
prop_getter_setter!(falloff_distance, f32, get_falloff_distance, set_falloff_distance);
prop_getter_setter!(falloff_buffer, f32, get_falloff_buffer, set_falloff_buffer);
prop_getter_setter!(shadow_falloff_gradient, f32,
    get_shadow_falloff_gradient, set_shadow_falloff_gradient);
prop_getter_setter!(shadow_falloff_distance, f32,
    get_shadow_falloff_distance, set_shadow_falloff_distance);

#[derive(Clone, Default)]
#[script_bind]
pub struct PointLightBuilder {
    position: Vector2f,
    props: Light2dProperties,
}

#[script_bind]
impl PointLightBuilder {
    #[script_bind]
    pub fn new() -> Self {
        Self::default()
    }

    #[script_bind]
    pub fn position(mut self, position: Vector2f) -> Self {
        self.position = position;
        self
    }

    #[script_bind]
    pub fn color(mut self, color: Vector3f) -> Self {
        self.props.color = color;
        self
    }

    #[script_bind]
    pub fn occludable(mut self, is_occludable: bool) -> Self {
        self.props.is_occludable = is_occludable;
        self
    }

    #[script_bind]
    pub fn intensity(mut self, intensity: f32) -> Self {
        self.props.intensity = intensity;
        self
    }

    #[script_bind]
    pub fn falloff_gradient(mut self, falloff_gradient: f32) -> Self {
        self.props.falloff_gradient = falloff_gradient;
        self
    }

    #[script_bind]
    pub fn falloff_distance(mut self, falloff_distance: f32) -> Self {
        self.props.falloff_distance = falloff_distance;
        self
    }

    #[script_bind]
    pub fn falloff_buffer(mut self, falloff_buffer: f32) -> Self {
        self.props.falloff_buffer = falloff_buffer;
        self
    }

    #[script_bind]
    pub fn shadow_falloff_gradient(mut self, shadow_falloff_gradient: f32) -> Self {
        self.props.shadow_falloff_gradient = shadow_falloff_gradient;
        self
    }

    #[script_bind]
    pub fn shadow_falloff_distance(mut self, shadow_falloff_distance: f32) -> Self {
        self.props.shadow_falloff_distance = shadow_falloff_distance;
        self
    }

    #[script_bind]
    pub fn build(mut self) -> PointLight {
        self.props.ty = Light2dType::Point;
        PointLight {
            properties: Dirtiable::new(self.props),
            transform: Dirtiable::new(Transform2d::new(
                self.position,
                Vector2f::new(0.0, 0.0),
                0.0,
            )),
            render_light: None,
        }
    }
}
