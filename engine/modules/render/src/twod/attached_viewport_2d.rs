use argus_core::ScreenSpaceScaleMode;
use argus_util::dirtiable::{Dirtiable, ValueAndDirtyFlag};
use argus_util::math::Vector2u;
use crate::common::{get_next_viewport_id, AttachedViewport, Matrix4x4, Transform2d, Viewport};
use crate::twod::get_render_context_2d;

#[derive(Clone, Copy, Debug)]
pub enum ViewportYAxisConvention {
    TopDown, // Vulkan
    BottomUp, // OpenGL
}

#[derive(Clone, Debug)]
pub struct AttachedViewport2d {
    id: u32,
    scene_id: String,
    camera_id: String,
    viewport: Viewport,
    z_index: u32,
    postfx_shaders: Vec<String>,
    view_matrix: Dirtiable<Matrix4x4>,
}

impl AttachedViewport for AttachedViewport2d {
    fn get_id(&self) -> u32 {
        self.id
    }

    fn get_viewport(&self) -> &Viewport {
        &self.viewport
    }

    fn get_z_index(&self) -> u32 {
        self.z_index
    }

    fn get_postprocessing_shaders(&self) -> &Vec<String> {
        &self.postfx_shaders
    }

    fn add_postprocessing_shader(&mut self, shader_uid: String) {
        self.postfx_shaders.push(shader_uid);
    }

    fn remove_postprocessing_shader(&mut self, shader_uid: String) {
        self.postfx_shaders.retain(|x| x != &shader_uid);
    }
}

impl AttachedViewport2d {
    #[must_use]
    pub(crate) fn new(
        scene_id: impl Into<String>,
        camera_id: impl Into<String>,
        viewport: Viewport,
        z_index: u32,
    ) -> Self {
        Self {
            id: get_next_viewport_id(),
            scene_id: scene_id.into(),
            camera_id: camera_id.into(),
            viewport,
            z_index,
            postfx_shaders: vec![],
            view_matrix: Dirtiable::new(Matrix4x4::identity()),
        }
    }

    pub fn get_scene_id(&self) -> &str {
        self.scene_id.as_str()
    }

    pub fn get_camera_id(&self) -> &str {
        self.camera_id.as_str()
    }

    pub fn is_view_state_dirty(&self) -> bool {
        self.view_matrix.peek().dirty
    }

    pub fn get_view_matrix(&mut self) -> ValueAndDirtyFlag<Matrix4x4> {
        self.view_matrix.read()
    }

    pub fn update_view_state(
        &mut self,
        resolution: &Vector2u,
        y_axis_convention: ViewportYAxisConvention,
    ) {
        self.update_view_matrix(resolution, y_axis_convention)
    }

    fn update_view_matrix(
        &mut self,
        resolution: &Vector2u,
        y_axis_convention: ViewportYAxisConvention,
    ) {
        let camera_transform = {
            let scene = get_render_context_2d().get_scene(self.get_scene_id()).unwrap();
            scene.get_camera(self.get_camera_id()).unwrap()
                .peek_transform()
        };
        self.view_matrix.update_in_place(|vm| {
            recompute_2d_viewport_view_matrix(
                &self.viewport,
                &camera_transform.inverse(),
                resolution,
                y_axis_convention,
                vm,
            );
        });
    }
}

fn compute_proj_matrix(res_hor: u32, res_ver: u32, y_convention: ViewportYAxisConvention)
    -> Matrix4x4 {
    // screen space is [0, 1] on both axes with the origin in either the top-left or bottom-left
    let l = 0;
    let r = 1;
    let (b, t) = match y_convention {
        // a sign is screwed up somewhere so these are the opposite of what you'd expect
        ViewportYAxisConvention::BottomUp => (1, 0),
        ViewportYAxisConvention::TopDown => (0, 1),
    };

    let res_hor_f = res_hor as f32;
    let res_ver_f = res_ver as f32;

    //let mode = EngineManager::instance().get_config().get_section::<>().screen_scale_mode;
    //TODO
    let mode = ScreenSpaceScaleMode::NormalizeMinDimension;
    let (hor_scale, ver_scale) = match mode {
        ScreenSpaceScaleMode::NormalizeMinDimension => {
            if res_hor > res_ver {
                (res_hor_f / res_ver_f, 1.0)
            } else {
                (1.0, res_ver_f / res_hor_f)
            }
        }
        ScreenSpaceScaleMode::NormalizeMaxDimension => {
            if res_hor > res_ver {
                (1.0, res_ver_f / res_hor_f)
            } else {
                (res_hor_f / res_ver_f, 1.0)
            }
        }
        ScreenSpaceScaleMode::NormalizeVertical => (res_hor_f / res_ver_f, 1.0),
        ScreenSpaceScaleMode::NormalizeHorizontal => (1.0, res_ver_f / res_hor_f),
        ScreenSpaceScaleMode::None => (1.0, 1.0),
    };

    Matrix4x4::from_row_major([
        2.0 / ((r - l) as f32 * hor_scale),
        0.0,
        0.0,
        -(r + l) as f32 / ((r - l) as f32 * hor_scale),
        0.0,
        2.0 / ((t - b) as f32 * ver_scale),
        0.0,
        -(t + b) as f32 / ((t - b) as f32 * ver_scale),
        0.0,
        0.0,
        1.0,
        0.0,
        0.0,
        0.0,
        0.0,
        1.0,
    ])
}

fn recompute_2d_viewport_view_matrix(
    viewport: &Viewport,
    transform: &Transform2d,
    resolution: &Vector2u,
    y_convention: ViewportYAxisConvention,
    dest: &mut Matrix4x4,
) {
    let center_x = (viewport.left + viewport.right) / 2.0;
    let center_y = (viewport.top + viewport.bottom) / 2.0;

    let mut adj_transform = transform.clone();
    adj_transform.translation.x += (viewport.right - viewport.left) / 2.0;
    adj_transform.translation.y += (viewport.bottom - viewport.top) / 2.0;

    let anchor_mat_1 = Matrix4x4::from_row_major([
        1.0,
        0.0,
        0.0,
        -center_x + adj_transform.translation.x,
        0.0,
        1.0,
        0.0,
        -center_y + adj_transform.translation.y,
        0.0,
        0.0,
        1.0,
        0.0,
        0.0,
        0.0,
        0.0,
        1.0,
    ]);
    let anchor_mat_2 = Matrix4x4::from_row_major([
        1.0,
        0.0,
        0.0,
        center_x - adj_transform.translation.x,
        0.0,
        1.0,
        0.0,
        center_y - adj_transform.translation.y,
        0.0,
        0.0,
        1.0,
        0.0,
        0.0,
        0.0,
        0.0,
        1.0,
    ]);

    let view_mat =
        adj_transform.get_translation_matrix() *
            anchor_mat_2 *
            adj_transform.get_rotation_matrix() *
            adj_transform.get_scale_matrix() *
            anchor_mat_1;

    *dest = compute_proj_matrix(resolution.x, resolution.y, y_convention) * view_mat;
}
