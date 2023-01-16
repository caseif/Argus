#version 460

uniform sampler2D u_Texture;
uniform vec2 u_UvStride;

in vec2 pass_TexCoord;
in vec2 pass_AnimFrame;

out vec4 out_Color;

void main() {
    float _;
    vec2 norm_tc = vec2(modf(pass_TexCoord.x, _), modf(pass_TexCoord.y, _));
    vec2 transformed_tc = (pass_AnimFrame + norm_tc) * u_UvStride;

    out_Color = texture(u_Texture, transformed_tc);
}
