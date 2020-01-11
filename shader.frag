#version 330
#extension GL_ARB_enhanced_layouts : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec4 tex_coords;
layout (location = 1) in vec4 tex_normal; 

layout (set = 0, binding = 0) uniform sampler2D r_Sampler0;
layout (set = 0, binding = 1) uniform sampler2D r_Sampler1;

// struct light_params
// {
    
// };

void main()
{
    gl_FragColor = texture(r_Sampler0, vec2(tex_coords.x, tex_coords.y));
    // gl_FragColor = normalize(tex_normal);
}