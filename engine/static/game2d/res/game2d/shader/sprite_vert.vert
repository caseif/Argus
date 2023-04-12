#version 460 core

layout(std140, binding = 3) uniform Global {
    float Time;
} global;

layout(std140, binding = 1) uniform Viewport {
    mat4 ViewMatrix;
} viewport;

layout(location = 0) in vec2 in_Position;
// tex coord attr is local to the current atlas "tile" and is generally (but not
// necessarily) expected to contain integer values
layout(location = 1) in vec2 in_TexCoord;
layout(location = 2) in vec2 in_AnimFrame;

out vec2 pass_TexCoord;
out vec2 pass_AnimFrame;

void main() {
    gl_Position = viewport.ViewMatrix * vec4(in_Position, 0.0, 1.0);
    pass_TexCoord = in_TexCoord;
    pass_AnimFrame = in_AnimFrame;
}
