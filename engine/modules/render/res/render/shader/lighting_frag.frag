#version 460 core

#define UINT_MAX 4294967295
#define PI 3.14159
#define TWO_PI (PI * 2.0)

#define LIGHTS_MAX 32

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
    bool is_occludable;
};

in vec2 WorldPos;
in vec2 TexCoord;

out vec4 out_Color;

//layout(binding = 0) uniform sampler2D u_Framebuffer;
layout(binding = 0) uniform usamplerBuffer u_RayBuffer;

layout(std140, binding = 0) uniform Scene {
    vec4 AmbientLightColor;
    float AmbientLightLevel;
} scene;

layout(std140, binding = 1) uniform Viewport {
    mat4 ViewMatrix;
    uint LightCount;
    Light2D Lights[32];
} viewport;

// Applies a power-like function with a non-integer exponent without doing an
// expensive pow() operation.
//
// This works quite well for base >= 1 but begins to diverge as base approaches
// 0, with the curve being less pronounced than for the actual function for a
// given exponent. However, precision isn't especially important here - all that
// matters are the general characteristics of the function.
float powerlike_fn(float base, float exponent) {
    float exp_fract;
    float exp_int = modf(exponent, exp_fract);
    return pow(base, exp_int + 1) / ((1 - exp_fract) * base + exp_fract);
}

float get_base_attenuation(Light2D light, float dist) {
    if (dist <= light.falloff_buffer) {
        return 1.0;
    } else if (dist >= light.falloff_buffer + light.falloff_distance) {
        return 0.0;
    } else {
        // distance from the edge of the falloff buffer
        float offset_dist = max(dist - light.falloff_buffer, 0.0);
        // This is roughly the slope { y = -(x / D)^G + 1 }
        // where D = falloff distance, G = falloff gradient, and x = offset_dist.
        //
        // If G == 1 this is a straight line through (0, 1) and (D, 0).
        // If G is a different value this is a non-linear slope through the same points.
        return -powerlike_fn(offset_dist / light.falloff_distance, light.falloff_gradient) + 1.0;
    }
}

float get_shadow_attenuation(Light2D light, float dist_from_occluder) {
    if (dist_from_occluder >= light.shadow_falloff_distance) {
        return 0.0;
    } else {
        // same equation but using the distance from the occluder and shadow-specific parameters
        return -powerlike_fn(dist_from_occluder / light.shadow_falloff_distance, light.shadow_falloff_gradient) + 1.0;
    }
}

void main() {
    uint ray_count = 720;

    vec3 light_sum = vec3(scene.AmbientLightColor.rgb * scene.AmbientLightLevel);

    for (int i = 0; i < LIGHTS_MAX; i++) {
        if (i >= viewport.LightCount) {
            break;
        }

        Light2D light = viewport.Lights[i];
        vec2 light_pos = light.position.xy;

        vec2 offset = WorldPos.xy - light.position.xy;
        float theta = atan(offset.y, offset.x) + PI;
        uint ray_index = uint(floor(float(ray_count) * theta / TWO_PI));

        float dist = distance(light.position.xy, WorldPos.xy);

        bool is_occluded = false;
        float occl_dist;
        if (light.is_occludable) {
            int ray_lookup_index = int(i * ray_count + ray_index);
            occl_dist = texelFetch(u_RayBuffer, ray_lookup_index).r / float(DIST_MULTIPLIER);
            is_occluded = dist >= occl_dist;
        }

        float base_attenuation = get_base_attenuation(light, dist);

        float attenuation;
        if (is_occluded) {
            float dist_from_occluder = dist - occl_dist;
            attenuation = base_attenuation * get_shadow_attenuation(light, dist_from_occluder);
        } else {
            attenuation = base_attenuation;
        }

        vec3 light_contrib = light.color.rgb * light.intensity * attenuation;
        light_sum = vec3(1) - (vec3(1) - light_sum) * (vec3(1) - light_contrib);
    }

    out_Color = vec4(light_sum, 1.0);
}
