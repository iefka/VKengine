#version 450
#include "general/common.glsl"

layout(location = 0) out vec2 fragOffset;
layout(location = 1) flat out int lightIndex;

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
