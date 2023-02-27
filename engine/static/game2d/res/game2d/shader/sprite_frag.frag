#version 460 core

uniform sampler2D u_Texture;
uniform vec2 u_UvStride;

in vec2 pass_TexCoord;
in vec2 pass_AnimFrame;

out vec4 out_Color;

void main() {
    vec2 norm_tc = vec2(fract(pass_TexCoord.x), fract(pass_TexCoord.y));
    vec2 transformed_tc = (pass_AnimFrame + norm_tc) * u_UvStride;

    out_Color = texture(u_Texture, transformed_tc);
}
