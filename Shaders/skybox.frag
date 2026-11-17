#version 450
#include "general/common.glsl"

layout(location = 0) in vec3 inViewDir;
layout(set = 1, binding = 0) uniform samplerCube skyboxSampler;
layout(location  = 0 ) out vec4 outColor;

void main(){

    outColor = texture(skyboxSampler, normalize(inViewDir));
}