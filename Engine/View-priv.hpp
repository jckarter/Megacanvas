//
//  View-priv.hpp
//  Megacanvas
//
//  Created by Joe Groff on 7/3/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#ifndef Megacanvas_View_priv_hpp
#define Megacanvas_View_priv_hpp

#include "Engine/Canvas.hpp"
#include "Engine/Layer.hpp"
#include "Engine/View.hpp"
#include "Engine/Util/NamedTuple.hpp"
#include "Engine/Util/GLMeta.hpp"

namespace Mega {    
    using ViewUniforms = UniformTuple<
        center,
        pixelAlign,
        tileCount,
        invTileCount,
        tilePhase,
        viewportScale,
        tileTrimSize,
        invTileTrimSize,
        tileTexLo,
        tileTexSize,
        mappingTextureScale,
        mappingTexture,
        tilesTexture>;
    
    using ViewVertex = NamedTuple<
        tileCoord<float[3]>,
        padding1<Pad<float>>,
        tileCorner<float[2]>,
        layerParallax<float[2]>,
        layerOrigin<float[2]>,
        padding2<Pad<float[2]>>>;
    static_assert(sizeof(ViewVertex) % 16 == 0, "ViewVertex should be 16-byte aligned");

    template<>
    struct Priv<View> {
        Canvas canvas;
        Vec center = Vec(0.0, 0.0);
        Vec pixelAlign = Vec(0.0, 0.0);
        double zoom = 1.0;
        double width = 0.0, height = 0.0;

        bool prepared:1, good:1;
        GLuint meshBuffer = 0, eltBuffer = 0, viewTileTotal = 0;
        GLuint meshArray = 0;
        GLuint viewTileCount[2] = {0, 0};
        GLuint tilesTexture = 0, mappingTexture = 0, tilesTextureCount = 0, mappingTextureSize = 0;
        GLuint fragShader = 0, vertShader = 0, program = 0;
        
        ViewUniforms uniforms;
        
        Priv(Canvas c);
        ~Priv();

        bool createProgram(GLuint *outFrag, GLuint *outVert, GLuint *outProg, std::string *outError);
        void deleteProgram();

        void updateShaderParams();
        void updateMesh();
        void updateCenter();
        void updateViewport();
        void updateZoom();
        
        void loadVisibleTiles();

        bool isTileLoaded(std::size_t tile);
    };
    
    constexpr GLuint TILES_TU = 0;
    constexpr GLuint MAPPING_TU = 1;
    
    extern char const *shaderPath;
}

#endif
