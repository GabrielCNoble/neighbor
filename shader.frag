#version 330
#extension GL_ARB_enhanced_layouts : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec4 tex_coords;
layout (location = 1) in vec4 tex_color;

layout (set = 1, binding = 0) uniform sampler2D test_tex;

void main()
{
    // gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0);
    // gl_FragColor = tex_color;
    // gl_FragColor = tex_coords;
    gl_FragColor = texture(test_tex, tex_coords.xy);
}