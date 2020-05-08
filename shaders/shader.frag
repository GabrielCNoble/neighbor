#version 330
#extension GL_ARB_enhanced_layouts : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec4 tex_coords;
layout (location = 1) in vec4 tex_normal;
layout (location = 2) in vec4 offset_size;

layout (set = 1, binding = 0) uniform sampler2D r_Sampler0;
// layout (set = 0, binding = 1) uniform sampler2D r_Sampler1;

// struct light_params
// {

// };

void main()
{
    vec4 color = texture(r_Sampler0, vec2(offset_size.x + offset_size.z * tex_coords.x,
                                            offset_size.y + offset_size.w * tex_coords.y));

    if(color.a < 0.2)
    {
        discard;
    }

    gl_FragColor = color;
}
