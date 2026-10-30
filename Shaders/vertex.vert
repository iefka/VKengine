#version 450

//in
layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec4 inNormal;
layout(location = 3) in vec2 inUV;

//out
layout(location = 0) out vec3 outColor;
layout(location = 1) out vec3 outWorldNormal;
layout(location = 2) out vec3 outWorldPos;
layout(location = 3) out vec2 outUV;
layout(location = 4) flat out uint outMaterialIndex;

struct GPUTransformData {
    mat4 modelMatrix;
    mat4 normalMatrix;
    uint materialIndex;
    uint _pad[3];
};

struct PointLight {
		vec4 lightPosition; //ignore w 
		vec4 lightColor;
		float radius;
	};

layout(std140, set = 0, binding = 0) uniform GlobalUbo{
    mat4 projectionMatrix;
    mat4 viewMatrix;
    mat4 inverceViewMatrix;
    mat4 invViewProjMatrix;   
    vec4 ambientLightColor;
    PointLight lights[10];
    int numLights;
} ubo;

layout(std430,set = 3, binding = 0) readonly buffer TransformBuffer{
    GPUTransformData transforms[];
} transformBuffer;



void main() {

    GPUTransformData t = transformBuffer.transforms[gl_InstanceIndex];

    vec4 worldPosition = t.modelMatrix * inPosition;
    gl_Position = ubo.projectionMatrix * ubo.viewMatrix * worldPosition;

    outWorldNormal = normalize(mat3(t.normalMatrix) * inNormal.xyz);
    outWorldPos = worldPosition.xyz;
    outColor = inColor.xyz;
    outUV = inUV;
    outMaterialIndex = t.materialIndex;
}