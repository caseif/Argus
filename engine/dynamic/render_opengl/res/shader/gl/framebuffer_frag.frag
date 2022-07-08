#version 420 core

layout(location=0) in vec2 pass_TexCoord;

layout(location=0) out vec4 out_Color;

uniform sampler2D screenTex;

void main() {
    out_Color = texture(screenTex, pass_TexCoord);
}
