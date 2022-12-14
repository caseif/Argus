#version 460

uniform mat4 u_ViewMatrix;
uniform vec2 u_UvStride;

layout(location = 0) in vec2 in_Position;
// tex coord attr is local to the current atlas "tile" and is expected to only
// contain values of 0 or 1
layout(location = 1) in vec2 in_TexCoord;
layout(location = 2) in vec2 in_AnimFrame;

out vec2 pass_TexCoord;

void main() {
    gl_Position = u_ViewMatrix * vec4(in_Position, 0.0, 1.0) + u_UvStride.x * 0.0000001;
    pass_TexCoord = (in_AnimFrame + in_TexCoord) * u_UvStride;
}
