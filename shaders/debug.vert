layout (location = 0) in vec4 r_VertexPosition;
layout (location = 1) in vec4 r_VertexColor;


void main()
{
    gl_Position = r_VertexPosition;
}
