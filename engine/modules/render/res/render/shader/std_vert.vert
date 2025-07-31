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

layout(std140, binding = 1) uniform Global {
    float Time;
} global;

layout(std140, binding = 3) uniform Viewport {
    mat4 ViewMatrix;
    uint LightCount;
    Light2D Lights[32];
} viewport;

layout(std140, binding = 4) uniform Object {
    vec2 UvStride;
    float LightOpacity;
} obj;

layout(location = 0) in vec2 in_Position;
// tex coord attr is local to the current atlas "tile" and is generally (but not
// necessarily) expected to contain integer values
layout(location = 1) in vec2 in_TexCoord;
layout(location = 2) in vec2 in_AnimFrame;

out vec2 TexCoord;
out vec2 AnimFrame;

void main() {
    gl_Position = viewport.ViewMatrix * vec4(in_Position, 0.0, 1.0);

    vec2 norm_tc = vec2(fract(in_TexCoord.x), fract(in_TexCoord.y));
    vec2 transformed_tc = (in_AnimFrame + in_TexCoord) * obj.UvStride;
    TexCoord = transformed_tc;

    AnimFrame = in_AnimFrame;
}
