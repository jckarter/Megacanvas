#version 150

uniform vec2 center, pixelAlign, tileCount;
uniform vec2 invTileCount; // 1/tileCount
uniform vec2 tilePhase; // 0.5*(tileCount - 1)
uniform vec2 viewportScale; // 2*zoom/viewport
uniform float tileTrimSize; // tileSize - 1
uniform float invTileTrimSize; // 1/(tileSize - 1)
uniform float tileTexLo; // 0.5/tileSize
uniform float tileTexSize; // (tileSize - 1)/tileSize
uniform float mappingTextureScale; // 1/(mappingTextureSegmentSize*2)
uniform sampler2DArray mappingTexture, tilesTexture;

in vec3 tileCoord;
in vec2 tileCorner, layerParallax, layerOrigin;

noperspective out vec3 frag_texCoord;

vec2 getTile(vec2 layerCenter) {
    vec2 baseTile = tileCoord.xy;
    vec2 centerTile = layerCenter * invTileTrimSize;
    return baseTile + tileCount * floor((centerTile - baseTile + tilePhase)*invTileCount);
}

float getMapping(vec2 tile, float layer) {
    vec3 mappingCoord = vec3((0.5 + tile)*mappingTextureScale, tileCoord.z);
    return texture(mappingTexture, mappingCoord).x * 65535.0;
}

void main() {
    vec2 layerCenter = (center - layerOrigin) * layerParallax + pixelAlign;
    vec2 tile = getTile(layerCenter);
    vec2 texCoord = tileTexLo + (tileCorner * tileTexSize);
    float texIndex = getMapping(tile, tileCoord.z);
    vec2 position = (tile + tileCorner)*tileTrimSize - layerCenter;
    gl_Position = vec4(position*viewportScale, 0.0, 1.0);
    frag_texCoord = vec3(texCoord, texIndex);
}