#version 420 core

layout(location=0) in vec2 in_Position;
layout(location=3) in vec2 in_TexCoord;

layout(location=0) out vec2 pass_TexCoord;

void main() {
    gl_Position = vec4(in_Position, 0.0, 1.0);
    pass_TexCoord = in_TexCoord;
}
