#version 460

in vec2 pass_TexCoord;

out vec4 out_Color;

uniform sampler2D u_Texture;

void main() {
    out_Color = texture(u_Texture, pass_TexCoord);
}
