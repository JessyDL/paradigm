#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 vsCol;
layout(location = 0) out vec4 fsCol;

void main() 
{
	fsCol = vec4(vsCol.xyz,1.0);	
}
