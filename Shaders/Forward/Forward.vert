#version 450
#extension GL_ARB_separate_shader_objects : enable

// TEMP diagnostic: use raw vertex position (no UBO) to isolate VBO vs UBO.
// If a small triangle appears in the center, the VBO data works.

layout(location = 0) in vec3 inPosition;
layout(location = 5) in vec3 inColor;

layout(location = 0) out vec3 vColor;

void main()
{
	gl_Position = vec4(inPosition, 1.0);
	vColor = inColor;
}
