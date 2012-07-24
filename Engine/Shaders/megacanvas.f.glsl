#version 150

//uniform sampler2DArray tilesTexture;

noperspective in vec3 frag_texCoord;

out vec4 color;

void main() {
    color = vec4(fract(frag_texCoord.xy*4096.0/256.0), 1.0, 1.0/(frag_texCoord.z+1.0));
}