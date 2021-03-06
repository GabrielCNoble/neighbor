#version 330
#extension GL_ARB_enhanced_layouts : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec4 r_VertexPosition;
layout (location = 1) in vec4 r_VertexNormal;
layout (location = 2) in vec4 r_VertexTexCoords;

layout (location = 0) out vec4 tex_coords;
layout (location = 1) out vec4 tex_normal;

layout (push_constant) uniform r_InputParams
{
    mat4 r_ModelViewProjectionMatrix;
};

void main()
{
    gl_Position =  r_ModelViewProjectionMatrix * r_VertexPosition;
    tex_coords = r_VertexTexCoords;
}
