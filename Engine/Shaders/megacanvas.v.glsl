#version 150

uniform vec2 center;
uniform vec2 viewport;
uniform float tilesTextureSize;

in vec2 position;
in vec2 layerOrigin;
in vec2 layerParallax;
in float layer;

noperspective out vec3 frag_texCoord;

void main() {
    vec2 layerCenter = (center - layerOrigin) * layerParallax;
    vec2 layerCoord = floor(layerCenter + position*0.5*viewport)/tilesTextureSize;
    
    frag_texCoord = vec3(layerCoord, layer);
    gl_Position = vec4(position, 0.0, 1.0);
}