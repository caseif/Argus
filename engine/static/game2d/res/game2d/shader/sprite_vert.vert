#version 460

uniform mat4 u_ViewMatrix;
uniform vec2 u_UvStride;

layout(location = 0) in vec2 in_Position;
// tex coord attr is local to the current atlas "tile" and is generally (but not
// necessarily) expected to contain integer values
layout(location = 1) in vec2 in_TexCoord;
layout(location = 2) in vec2 in_AnimFrame;

out vec2 pass_TexCoord;
out vec2 pass_AnimFrame;

void main() {
    gl_Position = u_ViewMatrix * vec4(in_Position, 0.0, 1.0);
    pass_TexCoord = in_TexCoord;
    pass_AnimFrame = in_AnimFrame;
}
