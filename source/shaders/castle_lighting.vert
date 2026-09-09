#version 450

// Per-object transforms. The member order matches UniformBufferObject in C++.
layout(set = 1, binding = 0) uniform ObjectUniforms {
    mat4 modelViewProjection;
    mat4 model;
    mat4 normalMatrix;
    vec4 surfaceSettings;
} objectData;

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexUV;

layout(location = 0) out vec3 worldPosition;
layout(location = 1) out vec3 worldNormal;
layout(location = 2) out vec2 textureUV;

void main() {
    vec4 positionInWorld = objectData.model * vec4(vertexPosition, 1.0);

    gl_Position = objectData.modelViewProjection * vec4(vertexPosition, 1.0);
    worldPosition = positionInWorld.xyz;
    worldNormal = normalize(mat3(objectData.normalMatrix) * vertexNormal);
    textureUV = vertexUV;
}
