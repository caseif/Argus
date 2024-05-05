#version 460 core

layout(location = 0) in vec2 in_Position;
layout(location = 1) in vec2 in_TexCoord;

out vec2 NormPos;
out vec2 TexCoord;

layout(std140, binding = 3) uniform Viewport {
    mat4 ViewMatrix;
} viewport;

void main() {
    gl_Position = vec4(in_Position, 0.0, 1.0);
    NormPos = (inverse(viewport.ViewMatrix) * gl_Position).xy;
    TexCoord = in_TexCoord;
}
