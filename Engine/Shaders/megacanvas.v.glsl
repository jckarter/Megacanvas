#version 150

uniform vec2 center, viewport, tileCount;
//FIXME uniform float zoom;
uniform float tileSize;
uniform float mappingTextureScale; // 1/(mappingTextureSegmentSize*2)
uniform sampler2DArray mappingTexture, tilesTexture;

in vec3 tileCoord;
in vec2 tileCorner, layerParallax, layerOrigin;

out noperspective vec3 frag_texCoord;

vec2 getTile(vec2 layerCenter) {
    vec2 baseTile = tileCoord.xy;
    vec2 centerTile = center / (tileSize - 1.0);
    return baseTile + tileCount * floor((centerTile - baseTile + 0.5*(centerTile - 1.0))/tileCount);
}

float getMapping(vec2 tile, float layer) {
    vec3 mappingCoord = vec3((0.5 + tile)*mappingTextureScale, tileCoord.z);
    return texture(mappingTexture, mappingCoord).x * 65535.0;
}

void main() {
    vec2 layerCenter = (center - layerOrigin) * layerParallax + 0.5;
    vec2 tile = getTile(layerCenter);
    vec2 texCoord = (0.5 + (tileCorner * (tileSize - 1.0)))/tileSize;
    float texIndex = getMapping(tile, tileCoord.z);
    vec2 position = (tile + tileCorner) * (tileSize - 1.0) - layerCenter;
    gl_Position = vec4(position, 0.0, 1.0);
    frag_texCoord = vec3(texCoord, texIndex);
}