#version 330
#extension GL_ARB_enhanced_layouts : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec4 tex_coords;
layout (location = 1) in vec4 tex_color;

layout (set = 0, binding = 0) uniform sampler2D r_Sampler0;
layout (set = 0, binding = 1) uniform sampler2D r_Sampler1;

// layout (set = 0, binding = 2) uniform sampler2D r_Sampler2;
// layout (set = 0, binding = 3) uniform sampler2D r_Sampler3;


void main()
{
    // gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0);
    // gl_FragColor = tex_color;
    // gl_FragColor = tex_coords;
    // gl_FragColor = texture(r_Sampler0, tex_coords.xy) * texture(r_Sampler1, tex_coords.xy);
    gl_FragColor = texture(r_Sampler0, vec2(tex_coords.x, tex_coords.y));
}