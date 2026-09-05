#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0, set = 1) uniform UniformBufferObject {
	mat4 mvpMat;
	mat4 mMat;
	mat4 normalMat;
	vec4 surfaceParams;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragUV;

void main() {
	gl_Position = ubo.mvpMat * vec4(inPosition, 1.0);
	fragPos = (ubo.mMat * vec4(inPosition, 1.0)).xyz;
	fragNormal = normalize(mat3(ubo.normalMat) * inNormal);
	fragUV  = inUV;
}
