#version 330
#extension GL_ARB_enhanced_layouts : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 tex_coords;

layout(set = 0, binding = 0) uniform sampler2D tex_sampler;

void main()
{
    gl_FragColor = texture(tex_sampler, tex_coords);
}

