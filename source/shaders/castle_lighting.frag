#version 450

const int MAX_POINT_LIGHTS = 16;
const float SHININESS = 48.0;

layout(location = 0) in vec3 worldPosition;
layout(location = 1) in vec3 worldNormal;
layout(location = 2) in vec2 textureUV;

layout(location = 0) out vec4 finalFragmentColor;

layout(set = 0, binding = 0) uniform SceneLighting {
    vec3 directionalLightDirection;
    // RGB: directional-light colour, A: ambient-light strength.
    vec4 directionalLightColor;
    vec3 cameraPosition;
    vec4 pointLightPositions[MAX_POINT_LIGHTS];
    vec4 pointLightColors[MAX_POINT_LIGHTS];
    // X stores the number of active point lights.
    vec4 pointLightSettings;
} scene;

layout(set = 1, binding = 0) uniform ObjectUniforms {
    mat4 modelViewProjection;
    mat4 model;
    mat4 normalMatrix;
    // X: texture scale, Y: planar mapping enabled, Z: albedo brightness.
    vec4 surfaceSettings;
} objectData;

layout(set = 1, binding = 1) uniform sampler2D albedoTexture;

vec2 chooseSurfaceUV(vec3 normal) {
    if (objectData.surfaceSettings.y <= 0.5) {
        return textureUV;
    }

    vec3 normalWeight = abs(normal);
    float scale = objectData.surfaceSettings.x;

    if (normalWeight.y >= normalWeight.x && normalWeight.y >= normalWeight.z) {
        return worldPosition.xz * scale;
    }
    if (normalWeight.x >= normalWeight.z) {
        return worldPosition.zy * scale;
    }
    return worldPosition.xy * scale;
}

vec3 pointLightContribution(int lightIndex, vec3 albedo,
                            vec3 normal, vec3 viewDirection) {
    vec3 offsetToLight = scene.pointLightPositions[lightIndex].xyz - worldPosition;
    float lightDistance = length(offsetToLight);
    vec3 lightDirection = offsetToLight / max(lightDistance, 0.001);

    float diffuseAmount = max(dot(normal, lightDirection), 0.0);
    float specularAmount = 0.0;
    if (diffuseAmount > 0.0) {
        vec3 halfwayDirection = normalize(viewDirection + lightDirection);
        specularAmount = pow(max(dot(normal, halfwayDirection), 0.0), SHININESS);
    }

    float attenuation = 1.0 /
        (1.0 + 0.12 * lightDistance + 0.045 * lightDistance * lightDistance);
    vec3 lightColor = scene.pointLightColors[lightIndex].rgb;

    vec3 diffuse = albedo * diffuseAmount * lightColor;
    vec3 specular = vec3(0.20) * specularAmount * lightColor;
    return (diffuse + specular) * attenuation;
}

void main() {
    vec3 normal = normalize(worldNormal);
    vec3 viewDirection = normalize(scene.cameraPosition - worldPosition);
    vec3 albedo = texture(albedoTexture, chooseSurfaceUV(normal)).rgb;
    albedo *= objectData.surfaceSettings.z;

    // Ambient light keeps surfaces readable even when no lamp directly reaches them.
    vec3 ambient = albedo * scene.directionalLightColor.a;

    vec3 lightDirection = normalize(-scene.directionalLightDirection);
    float diffuseAmount = max(dot(normal, lightDirection), 0.0);
    vec3 diffuse = albedo * diffuseAmount * scene.directionalLightColor.rgb;

    float specularAmount = 0.0;
    if (diffuseAmount > 0.0) {
        vec3 halfwayDirection = normalize(viewDirection + lightDirection);
        specularAmount = pow(max(dot(normal, halfwayDirection), 0.0), SHININESS);
    }
    vec3 specular = vec3(0.30) * specularAmount *
                    scene.directionalLightColor.rgb;

    vec3 pointLighting = vec3(0.0);
    int activePointLights = clamp(int(scene.pointLightSettings.x + 0.5),
                                  0, MAX_POINT_LIGHTS);
    for (int lightIndex = 0; lightIndex < activePointLights; ++lightIndex) {
        pointLighting += pointLightContribution(lightIndex, albedo,
                                                normal, viewDirection);
    }

    vec3 litColor = ambient + diffuse + specular + pointLighting;

    // A small distance-based exterior fog softens the flat ground horizon.
    // Interior coordinates are around Y = 1000, so the effect stays outdoors.
    if (scene.cameraPosition.y < 100.0) {
        float distanceToCamera = length(scene.cameraPosition - worldPosition);
        float fogAmount = smoothstep(45.0, 135.0, distanceToCamera) * 0.65;
        vec3 fogColor = vec3(0.030, 0.055, 0.110);
        litColor = mix(litColor, fogColor, fogAmount);
    }

    finalFragmentColor = vec4(clamp(litColor, 0.0, 1.0), 1.0);
}
