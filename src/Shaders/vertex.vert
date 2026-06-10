#version 450

//in
layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inColor;

//out
layout(location = 0) out vec4 outColor;

void main() {
    gl_Position = inPosition;
    outColor = inColor;
}