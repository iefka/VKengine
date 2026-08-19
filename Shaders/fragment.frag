#version 450
//in
layout(location = 0) in vec3 inColor;
layout(location = 1) in vec3 inWorldNormal;
layout(location = 2) in vec3 inWorldPos;

//out
layout(location = 0) out vec4 outColor;

layout(std140, set = 0, binding = 0) uniform GlobalUbo{
    mat4 projectionMatrix;
    mat4 viewMatrix;
    vec4 ambientLightColor;
    vec3 lightPosition;
    vec4 lightColor;
} ubo;

void main() 
{

     vec3 directionToLight = ubo.lightPosition - inWorldPos;
    float attenuation = 1.0 / dot(directionToLight, directionToLight);

    vec3 lightColor = ubo.lightColor.xyz * ubo.lightColor.w * attenuation;
    vec3 ambientLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;

    vec3 duffuseLight = lightColor * max(dot(normalize(inWorldNormal), normalize(directionToLight)), 0);

    outColor = vec4( (duffuseLight + ambientLight) * inColor,1.f);
}