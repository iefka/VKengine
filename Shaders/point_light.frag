#version 450
#include "general/common.glsl"


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