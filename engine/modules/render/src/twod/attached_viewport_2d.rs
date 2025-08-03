use crate::common::{get_next_viewport_id, AttachedViewport, Transform2d, Viewport};
use crate::twod::get_render_context_2d;
use argus_core::ScreenSpaceScaleMode;
use argus_util::dirtiable::{Dirtiable, ValueAndDirtyFlag};
use argus_util::math::{AABB, Vector2f, Vector2u, Matrix4x4};

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
    view_frustum: AABB,
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
            view_frustum: Default::default(),
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
    
    pub fn peek_view_matrix(&self) -> Matrix4x4 {
        self.view_matrix.peek().value
    }

    pub fn get_view_frustum(&self) -> &AABB {
        &self.view_frustum
    }

    pub fn update_view_state(
        &mut self,
        resolution: &Vector2u,
        y_axis_convention: ViewportYAxisConvention,
    ) {
        self.update_view_matrix(resolution, y_axis_convention);
        self.update_view_frustum(resolution);
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
            println!("camera transform: {:?}", camera_transform);
            recompute_2d_viewport_view_matrix(
                &self.viewport,
                &camera_transform.inverse(),
                resolution,
                y_axis_convention,
                vm,
            );
        });
    }

    fn update_view_frustum(
        &mut self,
        resolution: &Vector2u,
    ) {
        let camera_transform = {
            let scene = get_render_context_2d().get_scene(self.get_scene_id()).unwrap();
            scene.get_camera(self.get_camera_id()).unwrap()
                .peek_transform()
        };

        let viewport = &self.viewport;
        let vp_center_x = (viewport.left + viewport.right) / 2.0;
        let vp_center_y = (viewport.top + viewport.bottom) / 2.0;

        let res_hor = resolution.x;
        let res_ver = resolution.y;
        let res_hor_f = res_hor as f32;
        let res_ver_f = res_ver as f32;

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
        let vp_width = (viewport.right - viewport.left) / camera_transform.scale.x * hor_scale;
        let vp_height = (viewport.bottom - viewport.top) / camera_transform.scale.y * ver_scale;

        let center_x = vp_center_x + camera_transform.translation.x;
        let center_y = vp_center_y + camera_transform.translation.y;

        let half_width = vp_width / 2.0;
        let half_height = vp_height / 2.0;

        // corners prior to applying camera rotation
        let corners = [
            (center_x - half_width, center_y - half_height), // top-left
            (center_x + half_width, center_y - half_height), // top-right
            (center_x + half_width, center_y + half_height), // bottom-right
            (center_x - half_width, center_y + half_height), // bottom-left
        ];

        // apply rotation to each corner
        let rotation = camera_transform.rotation;
        let sin_rot = rotation.sin();
        let cos_rot = rotation.cos();

        let mut min_x = f32::MAX;
        let mut min_y = f32::MAX;
        let mut max_x = f32::MIN;
        let mut max_y = f32::MIN;

        for &(x, y) in &corners {
            let x_rel = x - center_x;
            let y_rel = y - center_y;

            let x_rot = x_rel * cos_rot - y_rel * sin_rot;
            let y_rot = x_rel * sin_rot + y_rel * cos_rot;

            let x_final = x_rot + center_x;
            let y_final = y_rot + center_y;

            min_x = min_x.min(x_final);
            min_y = min_y.min(y_final);
            max_x = max_x.max(x_final);
            max_y = max_y.max(y_final);
        }

        self.view_frustum =
            AABB::from_corners(Vector2f::new(min_x, min_y), Vector2f::new(max_x, max_y));
    }
}

fn compute_proj_matrix(res_hor: u32, res_ver: u32, y_convention: ViewportYAxisConvention)
    -> Matrix4x4 {
    // screen space is [0, 1] on both axes with the origin in either the top-left or bottom-left
    let l = 0;
    let r = 1;
    let (t, b) = match y_convention {
        // a sign is screwed up somewhere so these are the opposite of what you'd expect
        ViewportYAxisConvention::BottomUp => (0, 1),
        ViewportYAxisConvention::TopDown => (1, 0),
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
