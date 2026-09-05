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
    vLocalY = inPosition.y;
    vUV = inUV;
    gl_Position = ubo.uModelViewProjection * vec4(inPosition, 1.0);
}
