#version 450
#include "general/common.glsl"

layout(location = 0) out vec3 outViewDir;

void main(){
    vec2 pos = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2) * 2.0f - 1.0f;
    gl_Position = vec4(pos, 1.0f, 1.0f);

    vec4 worldPos = ubo.invViewProjMatrix * vec4(pos, 1.f, 1.f); 
    vec3 cameraPos = ubo.inverceViewMatrix[3].xyz;

    outViewDir = worldPos.xyz / worldPos.w - cameraPos;
}