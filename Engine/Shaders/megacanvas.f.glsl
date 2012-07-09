#version 150

uniform sampler2DArray tilesTexture;

in noperspective vec3 frag_texCoord;
out vec4 color;

void main() {
    color = texture(tilesTexture, frag_texCoord);
}