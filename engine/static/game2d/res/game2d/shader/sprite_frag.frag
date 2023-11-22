#version 460 core

layout(std140, binding = 3) uniform Global {
    float Time;
} global;

layout(std140, binding = 2) uniform Object {
    vec2 UvStride;
    float LightOpacity;
} obj;

layout(std140, binding = 4) uniform Scene {
    vec4 AmbientLightColor;
    float AmbientLightLevel;
} scene;

layout(binding = 0) uniform sampler2D u_Texture;

//in vec3 pass_FragColor;
in vec2 pass_TexCoord;
in vec2 pass_AnimFrame;

out vec4 out_Color;

void main() {
    vec2 norm_tc = vec2(fract(pass_TexCoord.x), fract(pass_TexCoord.y));
    vec2 transformed_tc = (pass_AnimFrame + norm_tc) * obj.UvStride;

    out_Color = texture(u_Texture, transformed_tc);
}
