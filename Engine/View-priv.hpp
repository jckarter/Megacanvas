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
#include "Engine/Util/StructMeta.hpp"
#include "Engine/Util/GLMeta.hpp"

namespace Mega {    
    
#define MEGA_FIELDS_ViewVertex(x)\
    x(position, float[2])\
    x(layerOrigin, float[2])\
    x(layerParallax, float[2])\
    x(layer, float)\
    x(_, float)
    
    MEGA_STRUCT(ViewVertex)
    
#define MEGA_FIELDS_ViewUniforms(x)\
    x(center, GLint)\
    x(viewport, GLint)\
    x(tilesTextureSize, GLint)
    
    MEGA_STRUCT(ViewUniforms)
    
    static_assert(sizeof(ViewVertex) % 16 == 0, "ViewVertex should be 16-byte aligned");

    template<>
    struct Priv<View> {
        Canvas canvas;
        Vec center;
        double zoom;
        Vec viewport;
        
        bool good = false;
        
        ViewUniforms uniforms;
        
        GLProgram program;
        GLTexture tilesTexture;
        GLBuffer meshBuffer, eltBuffer;
        GLVertexArray meshArray;
        FlipFlop<GLBuffer> pixelBuffers;
        
        std::size_t eltCount, tilesTextureSize;
        
        Priv(Canvas c);
        
        void updateMesh();
        void updateViewport();
        void updateCenter();
    };
    
    extern char const *shaderPath;
}

#endif
