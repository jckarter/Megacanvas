//
//  View.cpp
//  Megacanvas
//
//  Created by Joe Groff on 6/29/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#include "Engine/Canvas.hpp"
#include "Engine/Layer.hpp"
#include "Engine/View.hpp"

#include "Engine/Util/GL.h"
#include "Engine/Util/NamedTuple.h"

#include <cmath>

namespace Mega {
    using Uniforms = NamedTuple<>;
    using Vertex = NamedTuple<>;

    template<>
    struct Priv<View> {
        Canvas canvas;
        Vec center = Vec();
        double zoom = 1.0;
        double width = 0.0, height = 0.0;
        
        bool prepared = false;
        GLuint meshBuffer = 0, eltBuffer = 0, vertexCount = 0;
        GLuint tilesTexture = 0, mappingTexture = 0;
        GLuint fragShader = 0, vertShader = 0, program = 0;
        
        Priv(Canvas c);
        ~Priv();
        
        void createProgram(GLuint *outFrag, GLuint *outVert, GLuint *outProg);
        void deleteProgram();
        
        void updateMesh(double w, double h);
    };
    MEGA_PRIV_DTOR(View)
    
    Priv<View>::Priv(Canvas c)
    :
    canvas(c), center(0.0, 0.0), zoom(0.0), prepared(false),
    meshBuffer(0), eltBuffer(0), vertexCount(0),
    tilesTexture(0), mappingTexture(0),
    fragShader(0), vertShader(0), program(0)
    {}
    
    Priv<View>::~Priv()
    {
        if (this->prepared) {
            this->deleteProgram();
            glDeleteTextures(1, &this->tilesTexture);
            glDeleteBuffers(1, &this->eltBuffer);
            glDeleteBuffers(1, &this->meshBuffer);
        }
    }
    
    void Priv<View>::updateMesh()
    {
        using namespace std;
        double invTileSize = 1.0/double(this->canvas.tileSize());
        double invZoom = 1.0/this->zoom;
        size_t tilew = size_t(ceil(this->width*invZoom*invTileSize)) + 1, tileh = size_t(ceil(this->height*invZoom*invTileSize)) + 1;
        size_t tileCount = tilew * tileh;
        size_t vertexCount = this->canvas.layers().size() * tileCount;
        
        
    }
    
    Owner<View> View::create(Canvas c)
    {
        return createOwner<View>(c);
    }
    
    void View::prepare()
    {
        assert(!that->prepared);
        that->prepared = true;
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glGenBuffers(1, &that->meshBuffer);
        glGenBuffers(1, &that->eltBuffer);
        glGenTextures(1, &that->tilesTexture);
        glGenTextures(1, &that->mappingTexture);
        that->createProgram(&that->fragShader, &that->vertShader, &that->program);
    }
    
    void View::resize(double width, double height)
    {
        that->width = width;
        that->height = height;
        //viewport
        that->updateMesh();
    }
    
    void View::render()
    {
        //todo;
    }
    
    MEGA_PRIV_GETTER_SETTER(View, center, Vec)
    MEGA_PRIV_GETTER(View, zoom, double)
    
    void View::zoom(double x)
    {
        that->zoom = x;
        if (that->prepared)
            updateMesh();
    }
    
    Vec View::viewToCanvas(Vec viewPoint)
    {
        //todo;
    }
    
    Vec View::viewToLayer(Vec viewPoint, Layer l)
    {
        //todo;
    }
}