#version 450

layout(location = 0) out vec3 outViewDir;

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
    vec2 pos = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2) * 2.0f - 1.0f;
    gl_Position = vec4(pos, 1.0f, 1.0f);

    vec4 worldPos = ubo.invViewProjMatrix * vec4(pos, 1.f, 1.f); 
    vec3 cameraPos = ubo.inverceViewMatrix[3].xyz;

    outViewDir = worldPos.xyz / worldPos.w - cameraPos;
}