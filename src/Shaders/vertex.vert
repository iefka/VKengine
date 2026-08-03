#version 450

//in
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inUV;

//out
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform GlobalUbo{
    mat4 projectionView;
    vec3 lightDirecrtion;
} ubo;

layout(push_constant) uniform Push{
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;

const float AMBIENT  = 0.1f;

void main() {
    gl_Position = ubo.projectionView * push.modelMatrix * vec4(inPosition,1.f);

    vec3 normalWorldSpace = normalize(mat3(push.normalMatrix) * inNormal);

    float lightIntensity = AMBIENT + max(dot(normalWorldSpace, ubo.lightDirecrtion), 0);

    outColor = vec4( lightIntensity * inColor,1.f);
}