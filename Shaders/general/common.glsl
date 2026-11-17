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
