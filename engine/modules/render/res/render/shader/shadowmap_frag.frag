#version 460 core

#define PI 3.14159
#define TWO_PI (PI * 2.0)
#define FLOAT_MAX 3.402823e38

#define LIGHTS_MAX 32
#define RAY_COUNT 360

#define LIGHT_TYPE_POINT 0

#define DIST_MULTIPLIER 100000

struct Light2D {
    vec4 color;
    vec4 position;
    float intensity;
    float falloff_gradient;
    float falloff_distance;
    float falloff_buffer;
    float shadow_falloff_gradient;
    float shadow_falloff_distance;
    int type;
    uint is_occludable;
};

in vec2 WorldPos;
in vec2 TexCoord;

out vec4 out_Color;

layout(binding = 0) uniform sampler2D u_OpacityMap;

layout(std140, binding = 2) uniform Scene {
    vec4 AmbientLightColor;
    float AmbientLightLevel;
} scene;

layout(std140, binding = 3) uniform Viewport {
    mat4 ViewMatrix;
    mat4 ViewMatrixInverse;
    uint LightCount;
    Light2D Lights[32];
} viewport;

vec2 world_to_uv(vec2 world_pos) {
    return (viewport.ViewMatrix * vec4(world_pos, 0.0, 1.0)).xy * 0.5 + 0.5;
}

vec2 uv_to_world(vec2 uv) {
    return (viewport.ViewMatrixInverse * vec4(uv * 2.0 - 1.0, 0.0, 1.0)).xy;
}

void main() {
    int buf_len = LIGHTS_MAX * RAY_COUNT;
    int global_index = int(TexCoord.x * buf_len);
    uint light_index = uint(TexCoord.y * LIGHTS_MAX);

    if (light_index >= viewport.LightCount) {
        discard;
    }

    Light2D light = viewport.Lights[light_index];
    if (light.is_occludable == 0U) {
        discard;
    }

    uint ray_index = uint(TexCoord.x * RAY_COUNT);

    vec2 light_pos_uv = world_to_uv(light.position.xy);

    float theta = float(ray_index) * TWO_PI / float(RAY_COUNT) - PI;
    vec2 dir_world = vec2(cos(theta), sin(theta));
    vec2 dir_uv = (viewport.ViewMatrix * vec4(dir_world, 0.0, 0.0)).xy;
    vec2 dir_uv_unit = normalize(dir_uv);

    float max_world_dist = light.falloff_buffer + light.falloff_distance + light.shadow_falloff_distance;
    float max_uv_dist = max_world_dist * length(dir_uv) * 0.5;

    vec2 inv_dir = 1.0 / dir_uv_unit;

    vec2 t0 = (vec2(0.0) - light_pos_uv) * inv_dir;
    vec2 t1 = (vec2(1.0) - light_pos_uv) * inv_dir;

    vec2 t_min = min(t0, t1);
    vec2 t_max = max(t0, t1);

    float t_enter = max(max(t_min.x, t_min.y), 0.0);
    float t_exit  = min(t_max.x, t_max.y);

    if (t_enter > t_exit) {
        out_Color = vec4(FLOAT_MAX, 0.0, 0.0, 0.0);
        return;
    }

    ivec2 map_size = textureSize(u_OpacityMap, 0);

    float nearest = FLOAT_MAX;
    ivec2 last_texel = ivec2(-1);
    float step_uv = 1.0 / float(max(map_size.x, map_size.y));
    bool did_sample_in_frame = false;
    float t_end = min(t_exit, max_uv_dist);
    for (float dist_uv = t_enter; dist_uv <= t_end; dist_uv += step_uv) {
        vec2 sample_uv = light_pos_uv + dir_uv_unit * dist_uv;
        if (sample_uv.x <= 0.0 || sample_uv.y <= 0.0 || sample_uv.x >= 1.0 || sample_uv.y >= 1.0) {
            continue;
        }

        ivec2 sample_texel = ivec2(sample_uv * vec2(map_size));
        if (sample_texel == last_texel) {
            continue;
        }
        last_texel = sample_texel;

        did_sample_in_frame = true;

        if (texelFetch(u_OpacityMap, sample_texel, 0).r > 0.0) {
            // pixel is opaque to light
            vec2 sample_world = uv_to_world(sample_uv);
            float dist_world = distance(sample_world, light.position.xy);
            nearest = dist_world;
            break;
        }

        // else pixel is transparent to light; continue marching
    }

    out_Color = vec4(nearest, 0.0, 0.0, 0.0);
}
