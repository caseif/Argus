#version 460 core

in vec2 TexCoord;

out vec4 out_Color;

layout(binding = 0) uniform sampler2D u_Framebuffer;

layout(std140, binding = 1) uniform Scene {
    vec4 AmbientLightColor;
    float AmbientLightLevel;
} scene;

void main() {
    // get the base color of the pixel
    vec4 base_color = texture(u_Framebuffer, TexCoord);
    // then apply the ambient light color multiplicatively
    vec4 mult_color = base_color * vec4(scene.AmbientLightColor.rgb, 1.0);
    // finally scale it by the ambient light level
    out_Color = vec4(mult_color.rgb * scene.AmbientLightLevel, base_color.a);
}
