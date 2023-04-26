#version 460 core

layout(std140, binding = 3) uniform Global {
    float Time;
} global;

layout(std140, binding = 1) uniform Viewport {
    mat4 ViewMatrix;
} viewport;

layout(location = 0) in vec2 in_Position;
// tex coord attr is local to the current atlas "tile" and is generally (but not
// necessarily) expected to contain integer values
layout(location = 1) in vec2 in_TexCoord;
layout(location = 2) in vec2 in_AnimFrame;

//out vec3 pass_FragColor;
out vec2 pass_TexCoord;
out vec2 pass_AnimFrame;

/*vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);*/

void main() {
    gl_Position = viewport.ViewMatrix * vec4(in_Position, 0.0, 1.0);
    pass_TexCoord = in_TexCoord;
    pass_AnimFrame = in_AnimFrame;
    /*gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    pass_FragColor = colors[gl_VertexIndex];*/
}
