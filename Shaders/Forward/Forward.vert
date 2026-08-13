#version 450
#extension GL_ARB_separate_shader_objects : enable

// Forward vertex shader — reads per-instance LocalToWorld / Color
// from the GPU scene storage buffer via gl_InstanceIndex.

layout(set = 0, binding = 0, std140) uniform FrameUniforms
{
	mat4 View;
	mat4 Proj;
	mat4 ViewProj;
	vec3 CameraWorldPos;
	float Time;
} u_Frame;

struct Instance
{
	mat4 LocalToWorld;
	vec4 Color;
};

layout(set = 1, binding = 0, std430) readonly buffer SceneInstances
{
	Instance Instances[];
};

layout(location = 0) in vec3 inPosition;
layout(location = 5) in vec3 inColor;

layout(location = 0) out vec3 vColor;

void main()
{
	// gl_InstanceIndex already includes firstInstance from the draw command.
	uint i = gl_InstanceIndex;
	Instance inst = Instances[i];
	vec4 worldPos = inst.LocalToWorld * vec4(inPosition, 1.0);
	gl_Position = u_Frame.ViewProj * worldPos;
	vColor = inColor * inst.Color.rgb;
}
