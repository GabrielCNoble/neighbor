#version 330
#extension GL_ARB_enhanced_layouts : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec4 r_VertexPosition;
layout (location = 1) in vec4 r_VertexColor;

layout (location = 0) out vec4 color;

layout(push_constant) uniform r_Input
{
     mat4 r_ModelViewProjectionMatrix;
};

void main()
{
    gl_Position = r_ModelViewProjectionMatrix * r_VertexPosition;
    color = r_VertexColor;
}
