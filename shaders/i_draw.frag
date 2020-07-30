#version 330
#extension GL_ARB_enhanced_layouts : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 color;
layout(location = 1) in vec2 tex_coords;

layout(push_constant) uniform r_TextureParams
{
    layout(offset = 68) int textured;
};

layout(set = 0, binding = 0) uniform sampler2D r_Sampler0;

void main()
{
    vec4 frag_color = color;
    if(textured > 0)
    {
        frag_color *= texture(r_Sampler0, tex_coords);
        if(frag_color.a < 0.1)
        {
            discard;
        }
    }
    frag_color.r = pow(frag_color.r, 1.0/2.2);
    frag_color.g = pow(frag_color.g, 1.0/2.2);
    frag_color.b = pow(frag_color.b, 1.0/2.2);
    frag_color.a = pow(frag_color.a, 1.0/2.2);
    gl_FragColor = frag_color;
}
