#version 460 core

in vec2 pass_TexCoord;

out vec4 out_Color;

uniform sampler2D u_Framebuffer;

void main() {
    out_Color = texture(u_Framebuffer, pass_TexCoord);
}
