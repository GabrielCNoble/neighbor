#version 330
#extension GL_ARB_enhanced_layouts : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec4 r_VertexPosition;
layout (location = 1) in vec4 r_VertexNormal;
layout (location = 2) in vec4 r_VertexTexCoords;

layout (location = 0) out vec4 tex_coords;
layout (location = 1) out vec4 tex_normal;
layout (location = 2) out vec4 offset_size;

struct r_input_params_t
{
    mat4 model_view_projection_matrix;
    vec4 offset_size;
};

layout (set = 0, binding = 0) uniform r_Input
{
    r_input_params_t r_input_params[32];
};



void main()
{
    gl_Position =  r_input_params[gl_InstanceIndex].model_view_projection_matrix * r_VertexPosition;
    offset_size = r_input_params[gl_InstanceIndex].offset_size;
    tex_coords = r_VertexTexCoords;

//    gl_Position = r_VertexPosition;

    if(r_input_params[gl_InstanceIndex].offset_size.z < 0.0)
    {
        tex_coords.x = 1.0 - tex_coords.x;
        offset_size.z = -offset_size.z;
    }
}
