use argus_render::common::Transform2d;
use argus_scripting_bind::script_bind;
use argus_util::math::Vector2f;

const FP_EPSILON: f32 = 0.001;

#[derive(Clone, Copy, Debug)]
#[script_bind]
pub struct BoundingRect {
    pub size: Vector2f,
    pub center: Vector2f,
    pub rotation_rads: f32,
}

#[script_bind]
impl BoundingRect {
    #[script_bind]
    pub fn new(size: Vector2f, center: Vector2f, rotation_rads: f32) -> Self {
        Self {
            size,
            center,
            rotation_rads,
        }
    }

    pub fn transform(&self, transform: &Transform2d) -> Self {
        Self {
            size: self.size.clone() * transform.scale,
            center: self.center + transform.translation,
            rotation_rads: self.rotation_rads + transform.rotation,
        }
    }

    pub fn get_coordinates(&self) -> [Vector2f; 4] {
        let half_width = self.size.x / 2.0;
        let half_height = self.size.y / 2.0;
        let cos_theta = self.rotation_rads.cos();
        let sin_theta = self.rotation_rads.sin();
        let top_left = Vector2f::new(
            self.center.x - half_width * cos_theta + half_height * sin_theta,
            self.center.y - half_width * sin_theta - half_height * cos_theta,
        );
        let bottom_left = Vector2f::new(
            self.center.x - half_width * cos_theta - half_height * sin_theta,
            self.center.y - half_width * sin_theta + half_height * cos_theta,
        );
        let bottom_right = Vector2f::new(
            self.center.x + half_width * cos_theta - half_height * sin_theta,
            self.center.y + half_width * sin_theta + half_height * cos_theta,
        );
        let top_right = Vector2f::new(
            self.center.x + half_width * cos_theta + half_height * sin_theta,
            self.center.y + half_width * sin_theta - half_height * cos_theta,
        );

        [top_left, bottom_left, bottom_right, top_right]
    }

    pub fn adjust_movement_for_collision(&self, other: &BoundingRect, movement: &Vector2f)
        -> Option<(Vector2f, Option<Vector2f>)> {
        let a = self;
        let b = other;

        // early test to avoid expensive math for well-separated objects
        let centers_dist_squared = a.center.dist_squared(&b.center);
        let combined_radii_squared = ((a.size + b.size) / 2.0).mag_squared();
        if centers_dist_squared > combined_radii_squared {
            // collision is not possible
            return None;
        }
        // else collision might be possible (although not guaranteed)

        let coords_a = a.get_coordinates();
        let coords_b = b.get_coordinates();

        let mut start_a = 0;
        for i in 1..coords_a.len() {
            let cur_min = coords_a[start_a];
            let cur = coords_a[i];
            if cur.y < cur_min.y || ((cur.y - cur_min.y).abs() < FP_EPSILON && cur.x < cur_min.x) {
                start_a = i;
            }
        }

        let mut start_b = 0;
        for i in 1..coords_b.len() {
            let cur_min = -coords_b[start_b];
            let cur = -coords_b[i];
            if cur.y < cur_min.y || ((cur.y - cur_min.y).abs() < FP_EPSILON && cur.x < cur_min.x) {
                start_b = i;
            }
        }

        let mut last_vertex = coords_a[start_a] - coords_b[start_b];
        let mut mink_diff = vec![];
        let mut i = 0;
        let mut j = 0;
        let mut have_x_pos = false;
        let mut have_y_pos = false;
        let mut have_x_neg = false;
        let mut have_y_neg = false;
        while i < coords_a.len() || j < coords_b.len() {
            let edge_a = coords_a[(start_a + i + 1) % coords_a.len()] - coords_a[(start_a + i) % coords_a.len()];
            let edge_b = coords_b[(start_b + j) % coords_b.len()] - coords_b[(start_b + j + 1) % coords_b.len()];

            if i == coords_a.len() {
                // must take from B
                last_vertex += edge_b;
                j += 1;
            } else if j == coords_a.len() {
                // must take from A
                last_vertex += edge_a;
                i += 1;
            } else {
                let a_cross_b = edge_a.cross_mag(&edge_b);
                if a_cross_b <= 0.0 {
                    last_vertex += edge_a;
                    i += 1;
                }
                if a_cross_b >= 0.0 {
                    last_vertex += edge_b;
                    j += 1;
                }
            }

            if last_vertex.x > 0.0 {
                have_x_pos = true;
            } else if last_vertex.x < 0.0 {
                have_x_neg = true;
            } else {
                have_x_pos = true;
                have_x_neg = true;
            }
            if last_vertex.y > 0.0 {
                have_y_pos = true;
            } else if last_vertex.y < 0.0 {
                have_y_neg = true;
            } else {
                have_y_pos = true;
                have_y_neg = true;
            }

            mink_diff.push(last_vertex);
        }

        // convex polygon has to cross both axes in order to contain the origin
        if !(have_x_pos && have_x_neg && have_y_pos && have_y_neg) {
            // collision is not possible
            return None;
        }

        let mut collisions = Vec::new();

        for i in 0..mink_diff.len() {
            let p_0 = mink_diff[i];
            let p_1 = mink_diff[(i + 1) % mink_diff.len()];
            let edge = p_1 - p_0;

            let ray_cross_edge = (movement).cross_mag(&edge);
            if ray_cross_edge.abs() < FP_EPSILON {
                // edge is parallel to movement vector
                continue;
            } else if ray_cross_edge > -FP_EPSILON {
                // movement vector is not opposing collision normal
                // (MD polygon sides are only collidable coming from the "outside")
                continue;
            }

            let t = p_0.cross_mag(&edge) / ray_cross_edge;
            let u = p_0.cross_mag(&movement) / ray_cross_edge;

            if t >= -FP_EPSILON && t <= 1.0 + FP_EPSILON && u >= -FP_EPSILON && u <= 1.0 + FP_EPSILON {
                // return portion of movement vector before collision
                collisions.push((t, u, edge));
            }
        }

        if collisions.is_empty() {
            return None;
        }

        let (orig_t, u, edge) = collisions.into_iter()
            .min_by(|(t1, _, _), (t2, _, _)| t1.total_cmp(&t2))
            .unwrap();
        let t = (1.0 - orig_t).clamp(0.0, 1.0 - FP_EPSILON);

        let scaled_movement = if t > FP_EPSILON {
            *movement * t
        } else {
            Vector2f::new(0.0, 0.0)
        };

        let remaining_frac = orig_t.clamp(0.0, 1.0 - FP_EPSILON);
        let projected_mag = movement.dot(&edge) / edge.dot(&edge);

        if projected_mag.abs() < FP_EPSILON {
            return Some((scaled_movement, None));
        }

        let deflected_movement = edge * projected_mag * remaining_frac;

        return Some((scaled_movement, Some(deflected_movement)));
    }
}
