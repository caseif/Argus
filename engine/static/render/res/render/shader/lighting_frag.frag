#version 460 core

#define UINT_MAX 4294967295
#define PI 3.14159
#define TWO_PI (PI * 2.0)

#define LIGHTS_MAX 32

#define LIGHT_TYPE_POINT 0

#define DIST_MULTIPLIER 100000

struct Light2D {
    vec4 color;
    vec4 position;
    int type;
    float intensity;
    float attenuation_constant;
    bool is_occludable;
};

in vec2 NormPos;
in vec2 TexCoord;

out vec4 out_Color;

//layout(binding = 0) uniform sampler2D u_Framebuffer;
layout(binding = 0) uniform usamplerBuffer u_RayBuffer;

layout(std140, binding = 0) uniform Scene {
    vec4 AmbientLightColor;
    float AmbientLightLevel;
    uint LightCount;
    Light2D Lights[32];
} scene;

void main() {
    uint ray_count = 720;

    vec3 light_sum = vec3(scene.AmbientLightColor.rgb * scene.AmbientLightLevel);

    for (int i = 0; i < scene.LightCount; i++) {
        Light2D light = scene.Lights[i];
        vec2 light_pos = light.position.xy;

        vec2 offset = NormPos.xy - light.position.xy;
        float theta = atan(offset.y, offset.x) + PI;
        uint ray_index = uint(round(float(ray_count) * theta / TWO_PI));

        float dist = distance(light.position.xy, NormPos.xy);

        bool is_occluded = false;
        if (light.is_occludable) {
            int ray_lookup_index = int(i * ray_count + ray_index);
            float occl_dist = texelFetch(u_RayBuffer, ray_lookup_index).r / float(DIST_MULTIPLIER);
            is_occluded = dist >= occl_dist;
        }

        if (!is_occluded) {
            vec3 light_contrib = light.color.rgb * light.intensity / pow(dist + 1, light.attenuation_constant);
            light_sum = vec3(1) - (vec3(1) - light_sum) * (vec3(1) - light_contrib);
        }
    }

    out_Color = vec4(light_sum, 1.0);
}
