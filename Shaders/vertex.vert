#version 450

//in
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inUV;

//out
layout(location = 0) out vec3 outColor;
layout(location = 1) out vec3 outWorldNormal;
layout(location = 2) out vec3 outWorldPos;


layout(std140, set = 0, binding = 0) uniform GlobalUbo{
    mat4 projectionMatrix;
    mat4 viewMatrix;
    vec4 ambientLightColor;
    vec3 lightPosition;
    vec4 lightColor;
} ubo;

layout(push_constant) uniform Push{
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;


void main() {

    vec4 worldPosition = push.modelMatrix * vec4(inPosition,1.f);
    gl_Position = ubo.projectionMatrix * ubo.viewMatrix * worldPosition;

    outWorldNormal = normalize(mat3(push.normalMatrix) * inNormal);
    outWorldPos = worldPosition.xyz;
    outColor = inColor;
}