use std::any::Any;
use std::mem;
use std::ops::Index;
use argus_render::common::Transform2d;
use argus_scripting_bind::script_bind;
use argus_util::math::Vector2f;

const FP_EPSILON_2: f32 = 0.01;
const FP_EPSILON_3: f32 = 0.001;
const FP_EPSILON_4: f32 = 0.0001;

const T_BUFFER: f32 = 0.0;

#[derive(Clone, Copy, Debug)]
#[script_bind]
pub struct BoundingShape {
    pub ty: BoundingShapeType,
    pub size: Vector2f,
    pub center: Vector2f,
    pub rotation_rads: f32,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[script_bind]
pub enum BoundingShapeType {
    Rectangle,
    Circle,
    Capsule,
}

#[derive(Clone, Copy, Debug)]
pub struct CollisionResolution {
    pub collision_vector: Vector2f,
    pub allowed_movement: Vector2f,
    pub deflected_movement: Vector2f,
}

#[script_bind]
impl BoundingShape {
    #[script_bind]
    pub fn new(ty: BoundingShapeType, size: Vector2f, center: Vector2f, rotation_rads: f32)
        -> Self {
        Self {
            ty,
            size,
            center,
            rotation_rads,
        }
    }

    pub fn transform(&self, transform: &Transform2d) -> Self {
        Self {
            ty: self.ty,
            size: self.size.clone() * transform.scale,
            center: self.center + transform.translation,
            rotation_rads: self.rotation_rads + transform.rotation as f32,
        }
    }

    pub fn resolve_collision(
        &self,
        other: &BoundingShape,
        movement: &Vector2f,
        prev_resolution: Option<CollisionResolution>
    ) -> Option<CollisionResolution> {
        let Some(resolution) = find_collisions(self, other, movement, prev_resolution)
        else { return None; };

        //let t = (1.0 - orig_t).clamp(0.0, 1.0 - FP_EPSILON);

        /*let scaled_movement = if t > FP_EPSILON {
            let movement_adj = *movement * t;
            if movement_adj.x.abs() < FP_EPSILON && movement_adj.y.abs() < FP_EPSILON {
                Vector2f::new(0.0, 0.0)
            } else {
                movement_adj
            }
        } else {
            Vector2f::new(0.0, 0.0)
        };

        let remaining_frac = orig_t.clamp(0.0, 1.0 - FP_EPSILON);
        let projected_mag = movement.dot(&edge) / edge.dot(&edge);

        if projected_mag.abs() < FP_EPSILON {
            return Some((scaled_movement, None));
        }

        let deflected_movement = edge * projected_mag * remaining_frac;*/

        //return Some((scaled_movement, Some(deflected_movement)));
        Some(resolution)
    }
}

fn find_collisions(
    a: &BoundingShape,
    b: &BoundingShape,
    movement: &Vector2f,
    prev_resolution: Option<CollisionResolution>,
) -> Option<CollisionResolution> {
    return find_collisions_mpr(a, b, movement, prev_resolution);
}

fn find_collisions_explicit(a: &BoundingShape, b: &BoundingShape, movement: &Vector2f)
    -> Option<CollisionResolution> {
    if a.ty == BoundingShapeType::Rectangle && b.ty == BoundingShapeType::Rectangle {
        resolve_collision_rect_rect(a, b, movement)
    } else if a.ty == BoundingShapeType::Rectangle && b.ty == BoundingShapeType::Circle {
        todo!()
    } else {
        panic!("Not supported");
    }
}

fn find_collisions_gjk(a: &BoundingShape, b: &BoundingShape, movement: &Vector2f)
    -> Option<CollisionResolution> {
    // GJK stage

    let initial_dir = (b.center - a.center).norm();
    let initial_supp = support2(a, b, &initial_dir);
    let mut simplex = vec![initial_supp];
    let mut dir = -initial_supp.norm();

    println!("checking");
    let mut have_collision = false;
    loop {
        let supp = support2(a, b, &dir);
        //println!("{:?} - {:?} = {:?}", support(a, &dir), support(b, &-dir), supp);
        //println!("{:?} ⋅ {:?} = {:?}", supp, dir, supp.dot(&dir));
        //println!("DOT PRODUCT: {:?}", supp.dot(&dir));
        if supp.dot(&dir) < 0.0 {
            break;
        }
        let old_simplex = simplex.clone();
        simplex.push(supp);
        let (new_simplex, new_dir, contains_origin) = nearest_simplex(&simplex);
        have_collision = contains_origin;
        if !contains_origin && new_simplex == old_simplex && new_dir == dir {
            println!("simplex: {:?}", simplex);
            println!("dir: {:?}", dir);
            println!("PREVENTING INFINITE LOOP");
            have_collision = true;
            break;
        }

        if contains_origin {
            have_collision = true;
            break;
        }

        simplex = new_simplex;
        dir = new_dir;
    }

    if !have_collision {
        return None;
    }

    // EPA stage

    assert!(simplex.len() == 3);

    let mut pen_depth = 0.0;
    let mut pen_normal = Vector2f::default();

    loop {
        let mut closest_dist = f32::INFINITY;
        let mut earliest_t = f32::INFINITY;
        let mut closest_normal = Vector2f::default();
        let mut closest_index = 0;

        for i in 0..simplex.len() {
            let a = simplex[i];
            let b = simplex[(i + 1) % simplex.len()];

            //let n = (a * ab.dot(&ab) - ab * ab.dot(&a)).norm();
            //let d = n.dot(&a).abs();
            // CHATGPT SUGGESTION
            let ab = b - a;
            let mut n = Vector2f::new(ab.y, -ab.x).norm();
            if n.mag_squared() < 0.0001 {
                continue;
            }
            let d = n.dot(&a);
            if d < 0.0 {
                n = -n;
            }
            let n = n;
            let d = d.abs();
            let t = 1.0 - pen_depth / movement.dot(&n);

            if d < closest_dist && t <= earliest_t {
                println!("closest_dist: {}", closest_dist);
                println!("earliest_t: {}", earliest_t);
                closest_dist = d;
                earliest_t = t;
                closest_normal = n;
                closest_index = (i + 1) % simplex.len();
            }
        }

        let p = support2(a, b, &closest_normal);
        let d = p.dot(&closest_normal).abs();
        if simplex.iter().any(|v| v.eq_approx(&p, FP_EPSILON_3)) ||
            (d - closest_dist).abs() < 0.001 ||
            simplex.len() > 15 {
            if let Some(dupe) = simplex.iter().find(|v| v.eq_approx(&p, FP_EPSILON_3)) {
                println!("duplicate vertex (current: {:?}, new: {:?}", dupe, p);
            }
            println!("d = {}", d);
            println!("d - closest_dist = {}", d - closest_dist);
            println!("did {} EPA iterations", simplex.len() - 3);
            pen_depth = d;
            pen_normal = closest_normal;
            break;
        }

        simplex.insert(closest_index, p);
    }

    //println!("depth = {}, normal = {:?}", depth, normal);

    println!("movement: {:?}", movement);
    println!("movement mag: {:?}", movement.mag());
    println!("movement normal: {:?}", movement.norm());
    println!("pen normal: {:?}", pen_normal);
    println!("pen_depth: {:?}", pen_depth);
    let movement_on_normal = movement.dot(&pen_normal);
    println!("movement dot normal: {:?}", movement_on_normal);
    if pen_depth <= 0.0 && movement_on_normal.abs() < 0.001 {
        // movement is perpendicular to normal (i.e. parallel to tangent)
        return None;
    } else if movement_on_normal < 0.0 {
        // movement is in same direction as normal
        // (i.e. object already clipped somehow and is now moving out)
        return None;
    }

    let t = 1.0 - pen_depth / movement_on_normal;

    println!("raw t value: {}", pen_depth / movement_on_normal);
    println!("T VALUE: {}", t);

    if t < -T_BUFFER || t > 1.0 + T_BUFFER {
        return None;
    }
    //let t = t - FP_EPSILON_3;
    let pen_vec = pen_normal * pen_depth;
    let movement_adjusted = if t > T_BUFFER {
        *movement * t
    } else {
        Vector2f::new(0.0, 0.0)
    };
    //let movement_adjusted = *movement - (pen_normal * movement_on_normal * pen_depth);
    let movement_deflected = if t < 1.0 - T_BUFFER {
        let movement_remaining = *movement - movement_adjusted;
        //let movement_adjusted = *movement - pen_normal * movement.dot(&pen_normal);
        //Vector2f::new(pen_normal.y, -pen_normal.x) * (1.0 - movement_on_normal) * (1.0 - t)

        //let pen_tangent = Vector2f::new(pen_normal.y, -pen_normal.x);
        //pen_tangent * movement.dot(&pen_tangent) * (1.0 - t)
        // CHATGPT SUGGESTION
        movement_remaining - pen_normal * movement_remaining.dot(&pen_normal)
    } else {
        Vector2f::new(0.0, 0.0)
    };

    println!("pen_vec: {:?}", pen_vec);
    println!("movement_adjusted: {:?}", movement_adjusted);
    println!("movement_deflected: {:?}", movement_deflected);

    Some(CollisionResolution {
        collision_vector: pen_normal * pen_depth,
        allowed_movement: movement_adjusted,
        deflected_movement: movement_deflected,
    })

    /*let t = pen_depth / movement.mag();
    println!("t: {}", t);
    if t > -FP_EPSILON && t < 1.0 + FP_EPSILON {
        let tangent_vec = Vector2f::new(-pen_normal.y, pen_normal.x);
        //println!("tangent: {:?}", tangent_vec);
        return vec![(t, tangent_vec)];
    } else {
        return vec![];
    }*/
}

fn find_collisions_mpr(
    a: &BoundingShape,
    b: &BoundingShape,
    movement: &Vector2f,
    prev_resolution: Option<CollisionResolution>,
)
    -> Option<CollisionResolution> {
    // portal discovery

    println!("Portal Discovery");
    println!("----------------");

    println!("movement: {}", movement);

    let mut collision: Option<Vector2f> = None;

    let initial_dir = prev_resolution
        .map(|c| c.collision_vector)
        .unwrap_or(b.center - a.center);

    let (v, is_early_resolution, sc_collision) = discover_portal(a, b, movement, initial_dir);
    if is_early_resolution {
        collision = sc_collision;
    }

    println!("Initial v1 = {}", v[1]);
    println!("Initial v2 = {}", v[2]);

    if !is_early_resolution {
        collision = refine_portal(a, b, v[0], v[1], v[2]);
    }

    if let Some(pen_vec) = collision {
        let pen_normal = pen_vec.norm();
        let pen_depth = pen_vec.mag();
        println!("Penetration normal: {}", pen_normal);
        println!("Penetration depth: {}", pen_depth);

        let movement_on_normal = movement.dot(&pen_normal);
        println!("Movement on normal: {}", movement_on_normal);
        if pen_depth < FP_EPSILON_3 && movement_on_normal.abs() < FP_EPSILON_3 {
            // movement is perpendicular to normal (i.e. parallel to tangent)
            println!("Movement is tangential, allowing");
            return None;
        } else if movement_on_normal < FP_EPSILON_2 {
            println!("Movement is same direction as normal, allowing");
            // movement is in same direction as normal
            // (i.e. object already clipped somehow and is now moving out)
            return None;
        }

        let t = 1.0 - pen_depth / movement_on_normal - T_BUFFER;
        println!("T-value: {}", t);
        let movement_adjusted = if t > T_BUFFER {
            //*movement * t
            *movement - pen_vec
        } else {
            Vector2f::new(0.0, 0.0)
        };
        //let movement_adjusted = *movement - (pen_normal * movement_on_normal * pen_depth);
        let movement_deflected = /*if t < 1.0 - T_BUFFER*/ {
            let movement_remaining = *movement - movement_adjusted;
            //let movement_adjusted = *movement - pen_normal * movement.dot(&pen_normal);
            //Vector2f::new(pen_normal.y, -pen_normal.x) * (1.0 - movement_on_normal) * (1.0 - t)

            //let pen_tangent = Vector2f::new(pen_normal.y, -pen_normal.x);
            //let defl = pen_tangent * movement.dot(&pen_tangent);// * (1.0 - t);
            let movement_lost = pen_normal * movement_remaining.dot(&pen_normal) * (1.0 + T_BUFFER);
            let defl = movement_remaining - movement_lost;
            if &defl != movement {
                defl
            } else {
                Vector2f::new(0.0, 0.0)
            }
        /*} else {
            Vector2f::new(0.0, 0.0)*/
        };
        //vec![(movement_adjusted, Vector2f::new(0.0, 0.0))]
        Some(CollisionResolution {
            collision_vector: collision.unwrap().norm(),
            allowed_movement: movement_adjusted,
            deflected_movement: movement_deflected,
        })
    } else {
        println!("No collision detected");
        None
    }
}

fn discover_portal(a: &BoundingShape, b: &BoundingShape, movement: &Vector2f, initial_dir: Vector2f)
    -> ([Vector2f; 3], bool, Option<Vector2f>) {
    let mut v1;
    let mut v2;

    // step B: point deep inside B-A
    let v0 = a.center - b.center;
    //println!("v0: {}", v0);
    //let initial_dir = -v0.norm();
    println!("Initial dir: {}", initial_dir);

    // step C: normal from v0 towards origin
    println!("Computing initial v1");
    v1 = support2(a, b, &initial_dir);

    // step D: find normal of line between v0 (interior point) and v1 (support point)
    let support_edge = v1 - v0;
    let mut support_edge_normal = Vector2f::new(support_edge.y, -support_edge.x).norm();
    println!("Calculated support edge normal: {}", support_edge_normal);
    println!("s.e.n. \u{22c5} -v0 = {}", support_edge_normal.dot(&-v0));
    let sen_dot_v0 = support_edge_normal.dot(&-v0);
    if sen_dot_v0.abs() < FP_EPSILON_3 {
        // degenerate case where support edge is coincident with origin ray - fall back to simple
        // distance comparison where origin is inside shape if it is closer than support point
        println!("Support edge is coincident");
        if v0.mag_squared() < support_edge.mag_squared() {
            println!("HIT");
            let pen_vec = (v1 - v0).norm() * v1.mag();
            return ([v0, v1, Vector2f::new(0.0, 0.0)], true, Some(pen_vec));
        } else {
            println!("MISS");
            return ([v0, v1, Vector2f::new(0.0, 0.0)], true, None);
        }
    } else if sen_dot_v0 < 0.0 {
        // normal should lie on same side as the origin
        println!("Flipping support edge normal");
        support_edge_normal = -support_edge_normal;
    }
    let support_edge_normal = support_edge_normal;

    // step E: select second support point
    println!("Computing initial v2");
    v2 = support2(a, b, &support_edge_normal);

    ([v0, v1, v2], false, None)
}

fn refine_portal(a: &BoundingShape, b: &BoundingShape, v0: Vector2f, v1: Vector2f, v2: Vector2f)
    -> Option<Vector2f> {
    let mut v1 = v1;
    let mut v2 = v2;

    let mut have_collision = false;
    let mut iters = 0;
    println!();
    println!("Portal Refinement");
    println!("-----------------");
    for i in 0..15 {
        iters += 1;
        println!("ITERATION {}", iters);
        println!("  v1: {}", v1);
        println!("  v2: {}", v2);

        // step F: create portal between support points
        let portal = v2 - v1;

        if portal.is_zero() {
            //TODO: fix degenerate portal
            println!("  Portal is degenerate");
            break;
        }

        println!("  Portal: {}", portal);
        println!("  Portal x (v2 - v0): {}", portal.cross_mag(&(v2 - v0)));
        println!("  Portal x v0: {}", portal.cross_mag(&v0));

        // step G: create normal perp. to portal and pointing away from interior
        let mut portal_normal = Vector2f::new(portal.y, -portal.x).norm();
        if portal_normal.dot(&(v1 - v0)) < 0.0 {
            println!("  Flipping portal normal");
            portal_normal = -portal_normal;
        }
        let portal_normal = portal_normal;
        println!("  Portal normal: {}", portal_normal);

        // normal is pointing away from v0, so if normal is pointing away from origin then origin
        // must be inside A-B
        println!("  Portal normal \u{22c5} -v1 = {}", portal_normal.dot(&-v1));
        if portal_normal.dot(&-v1) <= 0.0 {
            // origin lies on same side of portal as v0, must be inside B-A
            have_collision = true;
            println!("  HIT (colliding)");
            println!("  Portal normal \u{22c5} (v0 - v1) = {}", portal_normal.dot(&(v0 - v1)));

            let mut pen_normal = Vector2f::new(portal.y, -portal.x).norm();
            if pen_normal.dot(&(v0 - v1)) > 0.0 {
                println!("  Flipping penetration normal");
                pen_normal = -pen_normal;
            }

            //let pen_depth = pen_normal.dot(&v2);
            let pen_depth = if v1.mag_squared() < v2.mag_squared() {
                v1.mag()
            } else {
                v2.mag()
            };

            return Some(pen_normal * pen_depth);
        }

        // compute third support point
        let v3 = support2(a, b, &portal_normal);
        println!("  v3: {}", v3);
        // determine if origin is outside support line through v3 perp. to portal normal (SAT)
        if portal_normal.dot(&-v3) > 0.0 {
            // no collision
            println!("  MISS (not colliding)");
            break;
        }

        // step H: determine which support edge the ray from the origin to v3 passes through
        if v3.eq_approx(&v1, FP_EPSILON_3) || v3.eq_approx(&v2, FP_EPSILON_3) {
            // we converged on a support point before detecting a collision, it's a miss
            println!("  MISS (converged on support point)");
            break;
        }

        if (v1 - v0).cross_mag(&-v0) * (v3 - v0).cross_mag(&-v0) < 0.0 {
            // ray passes between v1 and v3
            v2 = v3;
            println!("  Replaced v2 with {}", v3);
        } else {
            // ray passes between v3 and v2
            v1 = v3;
            println!("  Replaced v1 with {}", v3);
        }
    }
    println!("Did {} iterations", iters + 1);

    None
}

fn support2(a: &BoundingShape, b: &BoundingShape, dir: &Vector2f) -> Vector2f {
    println!("    Computing shape A support:");
    let sa = support(a, dir);
    println!("    Computing shape B support:");
    let sb = support(b, &-*dir);
    println!("    {} - {} = {}", sa, sb, sa - sb);
    sa - sb
}

fn support(shape: &BoundingShape, dir: &Vector2f) -> Vector2f {
    let mut dir = dir.norm();
    //if dir.x < FP_EPSILON_4 {
    //    dir.x = 0.0;
    //}
    //if dir.y < FP_EPSILON_4 {
    //    dir.y = 0.0;
    //}
    match shape.ty {
        BoundingShapeType::Rectangle => {
            let coord = get_rect_coordinates(&shape.center, &shape.size, shape.rotation_rads)
                .into_iter()
                .map(|v| (v, v.dot(&dir)))
                .max_by(|(_, d1), (_, d2)| (d1.partial_cmp(d2).unwrap()))
                .unwrap()
                .0;
            coord
        }
        BoundingShapeType::Circle => {
            println!("    center: {}", shape.center);
            println!("    dir: {}", dir);
            println!("    radius: {}", shape.size.x / 2.0);
            shape.center + dir * shape.size.x / 2.0
        }
        BoundingShapeType::Capsule => {
            let diameter = shape.size.x.min(shape.size.y);
            let length = shape.size.x.max(shape.size.y);
            let radius = diameter / 2.0;
            let seg_len = length - diameter;
            let (a, b) = if shape.size.y > shape.size.x {
                // vertical orientation
                (
                    Vector2f::new(shape.center.x, shape.center.y - seg_len / 2.0),
                    Vector2f::new(shape.center.x, shape.center.y + seg_len / 2.0),
                )
            } else {
                // horizontal orientation
                (
                    Vector2f::new(shape.center.x - seg_len / 2.0, shape.center.y),
                    Vector2f::new(shape.center.x + seg_len / 2.0, shape.center.y),
                )
            };

            let ab = b - a;
            let dy = dir.dot(&ab);
            let coord = if dy < 0.0 {
                a + dir * radius
            } else {
                b + dir * radius
            };
            coord
        },
    }
}

fn nearest_simplex(simplex: &[Vector2f]) -> (Vec<Vector2f>, Vector2f, bool) {
    //println!("get nearest simplex to {:?}", simplex);
    match *simplex {
        [a, b] => {
            //println!("2-simplex");
            let a0 = -a;
            let ab = b - a;
            let mut ab_perp = Vector2f::new(-ab.y, ab.x).norm();
            if ab_perp.dot(&a0) < 0.0 {
                ab_perp = -ab_perp;
            }
            return (vec![a, b], ab_perp, false);
        }
        [a, b, c] => {
            println!("3-simplex: {:?}", [a, b, c]);
            let axb = a.cross_mag(&b);
            let bxc = b.cross_mag(&c);
            let cxa = c.cross_mag(&a);
            println!("axb, bxc, cxa: {}, {}, {}", axb, bxc, cxa);
            let contains_origin =
                (axb >= -FP_EPSILON_4 && bxc >= -FP_EPSILON_4 && cxa >= -FP_EPSILON_4) ||
                    (axb <= FP_EPSILON_4 && bxc <= FP_EPSILON_4 && cxa <= FP_EPSILON_4);

            let edges = [(a, b), (b, c), (c, a)];
            let new_simplex = edges
                .into_iter()
                .map(|(a, b)| ([a, b], (b - a).cross_mag(&a).abs() / (b - a).mag_squared()))
                .min_by(|(_, d1), (_, d2)| d1.partial_cmp(&d2).unwrap())
                .unwrap()
                .0;
            let edge = new_simplex[1] - new_simplex[0];
            let mut edge_perp = Vector2f::new(edge.y, -edge.x).norm();
            if edge_perp.dot(&-new_simplex[0]) < 0.0 {
                edge_perp = -edge_perp;
            }
            println!("{:?}", (new_simplex.to_vec(), edge_perp, contains_origin));
            return (new_simplex.to_vec(), edge_perp, contains_origin);
        }
        _ => {
            panic!("Invalid simplex")
        }
    }
}

fn resolve_collision_rect_rect(a: &BoundingShape, b: &BoundingShape, movement: &Vector2f)
    -> Option<CollisionResolution> {
    assert!(a.ty == BoundingShapeType::Rectangle);
    assert!(b.ty == BoundingShapeType::Rectangle);

    // early test to avoid expensive math for well-separated objects
    let centers_dist_squared = a.center.dist_squared(&b.center);
    let combined_radii_squared = ((a.size + b.size) / 2.0).mag_squared();
    if centers_dist_squared > combined_radii_squared {
        // collision is not possible
        return None;
    }
    // else collision might be possible (although not guaranteed)

    let coords_a = get_rect_coordinates(&a.center, &a.size, a.rotation_rads);
    let coords_b = get_rect_coordinates(&b.center, &b.size, b.rotation_rads);

    let mut start_a = 0;
    for i in 1..coords_a.len() {
        let cur_min = coords_a[start_a];
        let cur = coords_a[i];
        if cur.y < cur_min.y || ((cur.y - cur_min.y).abs() < FP_EPSILON_3 && cur.x < cur_min.x) {
            start_a = i;
        }
    }

    let mut start_b = 0;
    for i in 1..coords_b.len() {
        let cur_min = -coords_b[start_b];
        let cur = -coords_b[i];
        if cur.y < cur_min.y || ((cur.y - cur_min.y).abs() < FP_EPSILON_3 && cur.x < cur_min.x) {
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
        if ray_cross_edge.abs() < FP_EPSILON_3 {
            // edge is parallel to movement vector
            continue;
        } else if ray_cross_edge > -FP_EPSILON_3 {
            // movement vector is not opposing collision normal
            // (MD polygon sides are only collidable coming from the "outside")
            continue;
        }

        let t = p_0.cross_mag(&edge) / ray_cross_edge;
        let u = p_0.cross_mag(&movement) / ray_cross_edge;

        if t >= -FP_EPSILON_3 && t <= 1.0 + FP_EPSILON_3 &&
            u >= -FP_EPSILON_3 && u <= 1.0 + FP_EPSILON_3 {
            // return portion of movement vector before collision
            collisions.push((t, edge));
        }
    }

    //collisions
    None
}

fn find_collisions_capsule_rect(a: &BoundingShape, b: &BoundingShape, movement: &Vector2f)
    -> Vec<(f32, Vector2f)> {
    assert!(a.ty == BoundingShapeType::Rectangle);
    assert!(b.ty == BoundingShapeType::Capsule);
    todo!()
}

fn get_rect_coordinates(center: &Vector2f, size: &Vector2f, rotation: f32) -> [Vector2f; 4] {
    let half_width = size.x / 2.0;
    let half_height = size.y / 2.0;
    let cos_theta = rotation.cos();
    let sin_theta = rotation.sin();
    let top_left = Vector2f::new(
        center.x - half_width * cos_theta + half_height * sin_theta,
        center.y - half_width * sin_theta - half_height * cos_theta,
    );
    let bottom_left = Vector2f::new(
        center.x - half_width * cos_theta - half_height * sin_theta,
        center.y - half_width * sin_theta + half_height * cos_theta,
    );
    let bottom_right = Vector2f::new(
        center.x + half_width * cos_theta - half_height * sin_theta,
        center.y + half_width * sin_theta + half_height * cos_theta,
    );
    let top_right = Vector2f::new(
        center.x + half_width * cos_theta + half_height * sin_theta,
        center.y + half_width * sin_theta - half_height * cos_theta,
    );

    [top_left, bottom_left, bottom_right, top_right]
}
