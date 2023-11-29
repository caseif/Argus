#version 410 core

in vec2 TexCoord;

out vec4 out_Color;

uniform sampler2D u_Framebuffer;

void main() {
    out_Color = texture(u_Framebuffer, TexCoord);
}
