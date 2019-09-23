#version 330
#extension GL_ARB_enhanced_layouts : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4 r_VertexPos;

layout(set = 0, binding = 0) uniform r_Input
{
    mat4 r_ModelViewProjectionMatrix;
}; 

void main()
{
    gl_Position =  r_ModelViewProjectionMatrix * r_VertexPos;
}