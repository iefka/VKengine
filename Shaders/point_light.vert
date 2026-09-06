#version 450

layout(location = 0) out vec2 fragOffset;
layout(location = 1) flat out int lightIndex;

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

const vec2 OFFSETS[6] = vec2[](
  vec2(-1.0, -1.0),
  vec2(-1.0, 1.0),
  vec2(1.0, -1.0),
  vec2(1.0, -1.0),
  vec2(-1.0, 1.0),
  vec2(1.0, 1.0)
);

float LIGHT_RADIUS = 0.05f;

void main(){

    fragOffset = OFFSETS[gl_VertexIndex % 6];
    lightIndex = gl_VertexIndex / 6;


   vec4 lightInCameraSpace = ubo.viewMatrix * vec4(ubo.lights[lightIndex].lightPosition.xyz,1.f);
   vec4 positionInCameaSpace = lightInCameraSpace + ubo.lights[lightIndex].radius * vec4(fragOffset,0.f,0.f);
   gl_Position = ubo.projectionMatrix * positionInCameaSpace;
}
