#version 460 core

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

layout(location = 0) in vec2 in_Position;
layout(location = 1) in vec2 in_TexCoord;

out vec2 WorldPos;
out vec2 TexCoord;

layout(std140, binding = 1) uniform Viewport {
    mat4 ViewMatrix;
    uint LightCount;
    Light2D Lights[32];
} viewport;

void main() {
    gl_Position = vec4(in_Position, 0.0, 1.0);
    WorldPos = (inverse(viewport.ViewMatrix) * gl_Position).xy;
    TexCoord = in_TexCoord;
}
