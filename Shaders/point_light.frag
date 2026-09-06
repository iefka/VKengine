#version 450

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

layout(set = 1, binding = 0) uniform sampler2D textureSampler;


layout(location = 0) in vec2 fragOffset;
layout(location = 1) flat in int lightIndex;

layout(location = 0) out vec4 outColor;

void main(){
    
    float dif = sqrt(dot(fragOffset,fragOffset));

    if(dif >= 1.f){
        discard;
    }
    outColor = vec4(ubo.lights[lightIndex].lightColor.xyz,1.f);
}