#version 450
#include "general/common.glsl"
#extension GL_EXT_nonuniform_qualifier : enable
//in
layout(location = 0) in vec3 inColor;
layout(location = 1) in vec3 inWorldNormal;
layout(location = 2) in vec3 inWorldPos;
layout(location = 3) in vec2 UVcoord;
layout(location = 4) flat in uint inMaterialIndex;

//out
layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D bindlessTextures[];

struct GPUMaterialData{
    vec4 diffuseColor;
    vec4 specularColor;
    uint diffuseTextureIndex;
    uint normalTextureIndex;
    uint _pad0;
    uint _pad1;
};

layout(std430, set = 2, binding = 0) readonly buffer MaterialBuffer{
    GPUMaterialData materials[];
}materialBuffer;

const uint INVALID_TEXTURE_INDEX = 0xFFFFFFFFu;


void main() {

    GPUMaterialData mat = materialBuffer.materials[inMaterialIndex];

    vec3 diffuseLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
    vec3 specularLight = vec3(0.0);
    vec3 surfaceNormal = normalize(inWorldNormal);
    vec3 cameraPos = ubo.inverceViewMatrix[3].xyz;
    vec3 viewDir = normalize(cameraPos - inWorldPos);

    // nonuniformEXT is required here: the index differs between neighboring
    // invocations/draws sharing the same descriptor array, without this
    // qualifier behavior is undefined per GL_EXT_nonuniform_qualifier spec.
    vec4 tex = texture(
    bindlessTextures[nonuniformEXT(mat.diffuseTextureIndex)],
    vec2(UVcoord.x, 1.f - UVcoord.y)
);

    vec3 color = inColor * tex.rgb * mat.diffuseColor.rgb;

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
        blinnTerm = pow(blinnTerm, mat.specularColor.a); // shininess (Ns) from material

        specularLight += intensity * blinnTerm * mat.specularColor.rgb;
  }


   //vec2 checkerUV = UVcoord * 10.0; // scale: try 4, 10, 20 to match tile density
   //float checker = mod(floor(checkerUV.x) + floor(checkerUV.y), 2.0);
   outColor = vec4(diffuseLight * color + specularLight, 1.0);
   //outColor = vec4(tex.rgb, 1.0);
}