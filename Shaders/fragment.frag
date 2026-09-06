#version 450
//in
layout(location = 0) in vec3 inColor;
layout(location = 1) in vec3 inWorldNormal;
layout(location = 2) in vec3 inWorldPos;
layout(location = 3) in vec2 UVcoord;

//out
layout(location = 0) out vec4 outColor;

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



void main() {


    vec3 diffuseLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
    vec3 specularLught = vec3(0.0);
    vec3 surfaceNormal = normalize(inWorldNormal);
    vec3 cameraPos = ubo.inverceViewMatrix[3].xyz;
    vec3 viewDir = normalize(cameraPos - inWorldPos);

    vec4 tex =  texture(textureSampler,UVcoord);
  
    vec3 color = inColor * tex.rgb;

    for (int i = 0; i < ubo.numLights; i++) {
        PointLight light = ubo.lights[i];
        vec3 directionToLight = light.lightPosition.xyz - inWorldPos;
        float attenuation = 1.0 / dot(directionToLight, directionToLight);
        directionToLight = normalize(directionToLight);

        float cosAngIncidence = max(dot(surfaceNormal, directionToLight), 0);
        vec3 intensity = light.lightColor.xyz * light.lightColor.w * attenuation;

        diffuseLight += intensity * cosAngIncidence;

        vec3 halfAngle = normalize(directionToLight + viewDir);
        float blinnTerm = dot(surfaceNormal,halfAngle);
        blinnTerm = clamp(blinnTerm,0,1);
        blinnTerm = pow(blinnTerm,512.0);

        specularLught += intensity * blinnTerm;
  }
  
  outColor = vec4(diffuseLight * color + specularLught * color, 1.0);
}