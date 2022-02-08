#version 300 es

precision mediump float;

in vec2 pass_TexCoord;

out vec4 out_Color;

uniform sampler2D screenTex;

void main() {
    out_Color = texture(screenTex, pass_TexCoord);
}
