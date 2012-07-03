#version 150

in vec2 frag_texcoord;
in vec4 frag_color;
out vec4 color;

void main()
{
    color = frag_color;
}