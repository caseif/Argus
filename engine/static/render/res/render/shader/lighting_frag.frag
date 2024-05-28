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
    uint falloff_gradient;
    float falloff_distance;
    float falloff_buffer;
    uint shadow_falloff_gradient;
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
    uint LightCount;
    Light2D Lights[32];
} scene;

void main() {
    uint ray_count = 720;

    vec3 light_sum = vec3(scene.AmbientLightColor.rgb * scene.AmbientLightLevel);

    for (int i = 0; i < scene.LightCount; i++) {
        Light2D light = scene.Lights[i];
        vec2 light_pos = light.position.xy;

        vec2 offset = WorldPos.xy - light.position.xy;
        float theta = atan(offset.y, offset.x) + PI;
        uint ray_index = uint(floor(float(ray_count) * theta / TWO_PI));

        float dist = distance(light.position.xy, WorldPos.xy);

        bool is_occluded = false;
        float occl_dist = dist;
        if (light.is_occludable) {
            int ray_lookup_index = int(i * ray_count + ray_index);
            occl_dist = texelFetch(u_RayBuffer, ray_lookup_index).r / float(DIST_MULTIPLIER);
            is_occluded = dist >= occl_dist;
        }

        float attenuation_denom;
        if (is_occluded) {
            float scaled_dist = occl_dist * light.falloff_distance;
            float falloff_offset = -light.falloff_buffer + 1.0;
            float atten_denom_at_occluder = pow(scaled_dist + falloff_offset, light.falloff_gradient);

            float dist_from_occluder = dist - occl_dist;
            float scaled_shadow_dist = dist_from_occluder * (light.shadow_falloff_distance + light.falloff_distance);
            float shadow_falloff_offset = 1.0;
            float shadow_mult = pow(scaled_shadow_dist + shadow_falloff_offset, light.shadow_falloff_gradient);

            attenuation_denom = max(atten_denom_at_occluder, 1.0) * max(shadow_mult, 1.0);
        } else {
            float scaled_dist = dist * light.falloff_distance;
            float falloff_offset = -light.falloff_buffer + 1.0;
            attenuation_denom = pow(scaled_dist + falloff_offset, light.falloff_gradient);
        }
        float attenuation = clamp(1.0 / attenuation_denom, 0.0, 1.0);

        vec3 light_contrib = light.color.rgb * light.intensity * attenuation;
        light_sum = vec3(1) - (vec3(1) - light_sum) * (vec3(1) - light_contrib);
    }

    out_Color = vec4(light_sum, 1.0);
}
