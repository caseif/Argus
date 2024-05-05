#version 460 core

#define PI 3.14159
#define TWO_PI (PI * 2.0)

#define LIGHTS_MAX 32

#define LIGHT_TYPE_POINT 0

struct Light2D {
    vec4 color;
    vec4 position;
    int type;
    float intensity;
    float decay_factor;
    bool is_occludable;
};

in vec2 NormPos;
in vec2 TexCoord;

out vec4 out_Color;

layout(binding = 0) uniform sampler2D u_OpacityMap;
layout(binding = 0, r32ui) uniform uimageBuffer u_RayBuffer;

layout(std140, binding = 2) uniform Scene {
    vec4 AmbientLightColor;
    float AmbientLightLevel;
    int LightCount;
    Light2D Lights[32];
} scene;

layout(std140, binding = 3) uniform Viewport {
    mat4 ViewMatrix;
} viewport;

void main() {
    if (scene.LightCount == 0) {
        discard;
    }

    if (texture(u_OpacityMap, TexCoord).r == 0.0) {
        // pixel is transparent to light
        discard;
    }

    uint ray_count = 360;

    for (uint i = 0; i < scene.LightCount; i++) {
        Light2D light = scene.Lights[i];
        vec2 light_pos = light.position.xy;

        vec2 offset = NormPos.xy - light_pos;
        float theta = atan(offset.y, offset.x) + PI;
        uint ray_index = uint(round(float(ray_count) * theta / TWO_PI));
        uint dist = uint(distance(light_pos, NormPos.xy) * 100000);
        imageAtomicMin(u_RayBuffer, int(i * ray_count + ray_index), dist);
    }

    discard;
}
