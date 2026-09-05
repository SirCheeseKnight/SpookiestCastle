#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

layout(binding = 1, set = 1) uniform sampler2D albedoMap;

layout(binding = 0, set = 0) uniform GlobalUniformBufferObject {
    vec3 lightDir;
    vec4 lightColor;
    vec3 eyePos;
    vec4 pointLightPos[6];
    vec4 pointLightColor[6];
} gubo;

layout(binding = 0, set = 1) uniform UniformBufferObject {
    mat4 mvpMat;
    mat4 mMat;
    mat4 normalMat;
    vec4 surfaceParams;
} ubo;

void main() {
    vec3 N = normalize(fragNormal);
    vec2 surfaceUV = fragUV;
    if (ubo.surfaceParams.y > 0.5) {
        vec3 absNormal = abs(N);
        if (absNormal.y >= absNormal.x && absNormal.y >= absNormal.z) {
            surfaceUV = fragPos.xz * ubo.surfaceParams.x;
        } else if (absNormal.x >= absNormal.z) {
            surfaceUV = fragPos.zy * ubo.surfaceParams.x;
        } else {
            surfaceUV = fragPos.xy * ubo.surfaceParams.x;
        }
    }
    vec3 albedo = texture(albedoMap, surfaceUV).rgb * ubo.surfaceParams.z;

    vec3 V = normalize(gubo.eyePos - fragPos);
    vec3 L = normalize(-gubo.lightDir);
    vec3 H = normalize(V + L);

    float NdotL = max(dot(N, L), 0.0);
    float specularFactor = 0.0;
    if (NdotL > 0.0) {
        specularFactor = pow(max(dot(N, H), 0.0), 48.0);
    }

    vec3 ambient = 0.09 * albedo;
    vec3 diffuse = albedo * NdotL * gubo.lightColor.rgb;
    vec3 specular = vec3(0.30) * specularFactor * gubo.lightColor.rgb;

    vec3 pointLighting = vec3(0.0);
    for (int i = 0; i < 6; ++i) {
        vec3 toLight = gubo.pointLightPos[i].xyz - fragPos;
        float distanceToLight = length(toLight);
        vec3 pointL = toLight / max(distanceToLight, 0.001);
        float pointDiffuse = max(dot(N, pointL), 0.0);

        float pointSpecular = 0.0;
        if (pointDiffuse > 0.0) {
            vec3 pointH = normalize(V + pointL);
            pointSpecular = pow(max(dot(N, pointH), 0.0), 48.0);
        }

        float attenuation = 1.0 /
            (1.0 + 0.09 * distanceToLight + 0.032 * distanceToLight * distanceToLight);
        pointLighting +=
            (albedo * pointDiffuse + vec3(0.20) * pointSpecular) *
            gubo.pointLightColor[i].rgb * attenuation;
    }

    vec3 color = clamp(ambient + diffuse + specular + pointLighting, 0.0, 1.0);

    outColor = vec4(color, 1.0);
}
