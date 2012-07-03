#version 150

in vec3 position;
in vec2 texcoord;
in vec4 color;

out vec2 frag_texcoord;
out vec4 frag_color;

void main()
{
    gl_Position = vec4(position, 1.0);
    frag_texcoord = texcoord;
    frag_color = color;
}