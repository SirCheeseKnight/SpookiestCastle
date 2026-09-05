#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;

layout(binding = 0, set = 0) uniform SkyUniformBufferObject {
    mat4 inverseViewProjection;
    vec2 resolution;
} sky;

// xyz is a fixed world-space direction; w is the star's angular radius.
const vec4 stars[30] = vec4[](
    vec4( 0.0000, 0.2588,  0.9659, 0.0018), vec4( 0.6830, 0.2588,  0.6830, 0.0022),
    vec4( 0.9659, 0.2588,  0.0000, 0.0016), vec4( 0.6830, 0.2588, -0.6830, 0.0020),
    vec4( 0.0000, 0.2588, -0.9659, 0.0018), vec4(-0.6830, 0.2588, -0.6830, 0.0022),
    vec4(-0.9659, 0.2588,  0.0000, 0.0016), vec4(-0.6830, 0.2588,  0.6830, 0.0020),
    vec4( 0.3245, 0.5299,  0.7835, 0.0018), vec4( 0.7835, 0.5299,  0.3245, 0.0022),
    vec4( 0.7835, 0.5299, -0.3245, 0.0016), vec4( 0.3245, 0.5299, -0.7835, 0.0020),
    vec4(-0.3245, 0.5299, -0.7835, 0.0018), vec4(-0.7835, 0.5299, -0.3245, 0.0022),
    vec4(-0.7835, 0.5299,  0.3245, 0.0016), vec4(-0.3245, 0.5299,  0.7835, 0.0020),
    vec4( 0.1116, 0.7660,  0.6330, 0.0018), vec4( 0.5645, 0.7660,  0.3074, 0.0022),
    vec4( 0.5923, 0.7660, -0.2497, 0.0016), vec4( 0.1741, 0.7660, -0.6188, 0.0020),
    vec4(-0.3752, 0.7660, -0.5219, 0.0018), vec4(-0.6420, 0.7660, -0.0320, 0.0022),
    vec4(-0.4253, 0.7660,  0.4820, 0.0016), vec4( 0.2149, 0.9272,  0.3069, 0.0020),
    vec4( 0.3582, 0.9272, -0.1095, 0.0018), vec4( 0.0065, 0.9272, -0.3745, 0.0022),
    vec4(-0.3542, 0.9272, -0.1220, 0.0016), vec4(-0.2254, 0.9272,  0.2992, 0.0020),
    vec4( 0.1392, 0.9903,  0.0000, 0.0018), vec4(-0.1392, 0.9903,  0.0000, 0.0022)
);

const vec3 moonDirection = vec3(0.1000, 0.6200, -0.7780);

void main() {
    vec2 safeResolution = max(sky.resolution, vec2(1.0));
    vec2 uv = gl_FragCoord.xy / safeResolution;
    vec2 ndc = uv * 2.0 - 1.0;

    vec4 worldPoint = sky.inverseViewProjection * vec4(ndc, 1.0, 1.0);
    vec3 worldDirection = normalize(worldPoint.xyz / worldPoint.w);

    vec3 topColor = vec3(0.003, 0.007, 0.028);
    vec3 horizonColor = vec3(0.035, 0.065, 0.145);
    float skyHeight = smoothstep(0.0, 0.85, clamp(worldDirection.y, 0.0, 1.0));
    vec3 color = mix(horizonColor, topColor, skyHeight);

    float starLight = 0.0;
    for (int i = 0; i < 30; ++i) {
        float starDistance = length(worldDirection - normalize(stars[i].xyz));
        starLight += 1.0 - smoothstep(stars[i].w * 0.4, stars[i].w, starDistance);
    }
    color += vec3(0.68, 0.78, 1.0) * min(starLight, 1.0);

    float moonDistance = length(worldDirection - normalize(moonDirection));
    float moon = 1.0 - smoothstep(0.074, 0.084, moonDistance);
    color = mix(color, vec3(1.0, 0.94, 0.72), moon);

    outColor = vec4(color, 1.0);
}
