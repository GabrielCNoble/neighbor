#version 330
#extension GL_ARB_enhanced_layouts : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 color;

void main()
{
    gl_FragColor = color;
}
