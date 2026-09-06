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
layout(location = 3) out vec2 outUV;

struct PointLight {
		vec4 lightPosition; //ignore w 
		vec4 lightColor;
		float radius;
	};

layout(std140, set = 0, binding = 0) uniform GlobalUbo{
    mat4 projectionMatrix;
    mat4 viewMatrix;
    mat4 inverceViewMatrix;
    vec4 ambientLightColor;
    PointLight lights[10];
    int numLights;
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
    outUV = inUV;
}