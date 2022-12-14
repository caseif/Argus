#version 460

uniform mat4 u_ViewMatrix;

in vec2 in_Position;
in vec2 in_TexCoord;

out vec2 pass_TexCoord;

void main() {
    gl_Position = u_ViewMatrix * vec4(in_Position, 0.0, 1.0);
    pass_TexCoord = in_TexCoord;
}
