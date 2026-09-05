#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragUV;

layout(location = 0) out vec4 outColor;
layout(binding = 1, set = 1) uniform sampler2D albedoMap;

void main() {
    // Sample the base texture (optional, if you want texture details)
    vec4 texColor = texture(albedoMap, fragUV);

    // Choose a glowing ghostly color (e.g., cyan/greenish)
    vec3 glowColor = vec3(0.4, 0.9, 0.8);

    // Combine texture with the glow and set alpha to 0.6 for transparency
    outColor = vec4(glowColor * texColor.rgb + glowColor * 0.5, 0.4);
}


