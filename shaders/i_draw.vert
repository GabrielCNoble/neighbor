#version 330
#extension GL_ARB_enhanced_layouts : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec4 r_VertexPosition;
layout (location = 1) in vec4 r_VertexTexCoords;
layout (location = 2) in vec4 r_VertexColor;

layout (location = 0) out vec4 color;
layout (location = 1) out vec2 tex_coords;

layout(push_constant) uniform r_Input
{
     mat4 r_ModelViewProjectionMatrix;
     float r_PointSize;
};

void main()
{
    gl_Position = r_ModelViewProjectionMatrix * r_VertexPosition;
    gl_PointSize = r_PointSize;
    color = r_VertexColor;
    tex_coords = r_VertexTexCoords.xy;
}
