#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(set = 1, binding = 0) uniform UniformBufferObject {
    mat4 uModelViewProjection;
    mat4 mMat;
    mat4 normalMat;
    vec4 surfaceParams;
} ubo;

layout(location = 0) out float vLocalY;
layout(location = 1) out vec2 vUV;

void main() {
    float time = ubo.surfaceParams.x;

    // Calculate a smooth up-and-down floating offset using a sine wave
    float floatOffset = sin(time / 13.0 + inPosition.x * 2.0) * 0.15;

    vec3 modifiedPosition = inPosition;
    modifiedPosition.y += floatOffset; // Apply float to local height

    vLocalY = inPosition.y;
    vUV = inUV;
    gl_Position = ubo.uModelViewProjection * vec4(modifiedPosition, 1.0);
}
