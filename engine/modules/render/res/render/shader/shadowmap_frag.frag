#version 460 core

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

layout(binding = 0) uniform sampler2D u_OpacityMap;
layout(binding = 0, r32ui) uniform uimageBuffer u_RayBuffer;

layout(std140, binding = 2) uniform Scene {
    vec4 AmbientLightColor;
    float AmbientLightLevel;
} scene;

layout(std140, binding = 3) uniform Viewport {
    mat4 ViewMatrix;
    uint LightCount;
    Light2D Lights[32];
} viewport;

void main() {
    if (viewport.LightCount == 0) {
        discard;
    }

    if (texture(u_OpacityMap, TexCoord).r == 0.0) {
        // pixel is transparent to light
        discard;
    }

    uint ray_count = 720;

    for (int i = 0; i < LIGHTS_MAX; i++) {
        if (i >= viewport.LightCount) {
            break;
        }

        Light2D light = viewport.Lights[i];
        vec2 light_pos = light.position.xy;
        vec2 offset = WorldPos.xy - light_pos;

        if (abs(offset.x) < 0.01 && abs(offset.y) < 0.01) {
            // light is fully occluded
            for (int j = 0; j < ray_count; j++) {
                imageAtomicExchange(u_RayBuffer, int(i * ray_count + j), 0);
            }
            continue;
        }

        float theta = atan(offset.y, offset.x) + PI;
        uint ray_index = uint(floor(float(ray_count) * theta / TWO_PI));
        uint dist = uint(distance(light_pos, WorldPos.xy) * DIST_MULTIPLIER);
        imageAtomicMin(u_RayBuffer, int(i * ray_count + ray_index), dist);
    }

    discard;
}
