#version 460 core

layout(std140, binding = 1) uniform Global {
    float Time;
} global;

layout(std140, binding = 2) uniform Scene {
    vec4 AmbientLightColor;
    float AmbientLightLevel;
} scene;

layout(std140, binding = 4) uniform Object {
    vec2 UvStride;
    float LightOpacity;
} obj;

layout(binding = 0) uniform sampler2D u_Texture;

in vec2 pass_TexCoord;
in vec2 pass_AnimFrame;

out vec4 out_Color;

void main() {
    out_Color = texture(u_Texture, pass_TexCoord);
}
