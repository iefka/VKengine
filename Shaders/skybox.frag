#version 450

layout(location = 0) in vec3 inViewDir;
layout(set = 1, binding = 0) uniform samplerCube skyboxSampler;
layout(location  = 0 ) out vec4 outColor;

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

void main(){

    outColor = texture(skyboxSampler, normalize(inViewDir));
}