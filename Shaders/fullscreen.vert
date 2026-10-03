#version 450

layout(location = 0) out vec2 outUV;

void main(){

	vec2 pos = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2) * 2.f - 1.f;
	gl_Position = vec4(pos, 0.f, 1.f);
	outUV = pos * 0.5f + 0.5f;
}