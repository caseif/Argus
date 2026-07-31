#version 460 core

in vec2 TexCoord;

out vec4 out_Color;

layout(binding = 0) uniform sampler2D u_FrameBuffer;

void main() {
    out_Color = texture(u_FrameBuffer, TexCoord);
}
