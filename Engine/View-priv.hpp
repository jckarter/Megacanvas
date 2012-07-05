//
//  View-priv.hpp
//  Megacanvas
//
//  Created by Joe Groff on 7/3/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#ifndef Megacanvas_View_priv_hpp
#define Megacanvas_View_priv_hpp

#include "Engine/View.hpp"
#include "Engine/Util/NamedTuple.hpp"
#include "Engine/Util/GLMeta.hpp"

namespace Mega {
    using ViewUniforms = NamedTuple<>;
    using ViewVertex = NamedTuple<
        tileCoord<float[3]>,
        padding1<Pad<float>>,
        tileCorner<float[2]>,
        layerParallax<float[2]>,
        layerOrigin<float[2]>,
        padding2<Pad<float[2]>>>;

    template<>
    struct Priv<View> {
        Canvas canvas;
        Vec center = Vec();
        double zoom = 1.0;
        double width = 0.0, height = 0.0;

        bool prepared = false;
        GLuint meshBuffer = 0, eltBuffer = 0, viewTileCount = 0;
        GLuint tilesTexture = 0, mappingTexture = 0;
        GLuint fragShader = 0, vertShader = 0, program = 0;

        Priv(Canvas c);
        ~Priv();

        void createProgram(GLuint *outFrag, GLuint *outVert, GLuint *outProg);
        void deleteProgram();

        void updateMesh();
    };
}

#endif
