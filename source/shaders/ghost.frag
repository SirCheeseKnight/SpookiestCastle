#version 450

layout(location = 0) in float vLocalY;
layout(location = 1) in vec2 vUV;
layout(location = 0) out vec4 FragColor;

layout(set = 1, binding = 1) uniform sampler2D texSampler;

void main() {
    float minY = -0.5;
    float maxY =  1.0;

    float normalizedHeight = (vLocalY - minY) / (maxY - minY);

    float alphaMultiplier = clamp(normalizedHeight, 0.0, 1.0);

    vec4 texColor = texture(texSampler, vUV);
    vec3 finalRgb = texColor.rgb * vec3(0.5, 0.8, 1.2);
    float finalAlpha = texColor.a * alphaMultiplier * 0.85;

    if (finalAlpha < 0.01) {
        discard;
    }

    FragColor = vec4(finalRgb, finalAlpha);
}
