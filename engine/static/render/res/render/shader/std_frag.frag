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

in vec2 TexCoord;
in vec2 AnimFrame;

layout(location = 0) out vec4 out_Color;
layout(location = 1) out vec4 out_LightOpacity;

void main() {
    vec4 color = texture(u_Texture, TexCoord);
    out_Color = color;
    //out_LightOpacity = vec4(obj.LightOpacity * color.a, 0.0, 0.0, 0.0);
    out_LightOpacity = vec4(1.0, 0.0, 1.0, 0.5);
}
