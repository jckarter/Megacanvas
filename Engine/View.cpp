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

#include <cmath>

namespace Mega {
    template<>
    struct Priv<View> {
        Canvas canvas;
        Vec center;
        double zoom;
        
        bool prepared;
        GLuint meshBuffer, eltBuffer, vertexCount;
        GLuint tilesTexture, mappingTexture;
        GLuint fragShader, vertShader, program;
        struct {
            
        } uniform;
        struct {
            
        } vertex;
        
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
    
    void Priv<View>::updateMesh(double w, double h)
    {
        using namespace std;
        double invTileSize = 1.0/double(this->canvas.tileSize());
        size_t tilew = size_t(ceil(w*invTileSize)) + 1, tileh = size_t(ceil(w*invTileSize)) + 1;
        size_t vertexCount = this->canvas.layers().size() * tilew * tileh;
        
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
        that->createProgram(&that->fragShader, &that->vertShader, &that->program);
    }
    
    void View::resize(double width, double height)
    {
        that->updateMesh(width, height);
    }
    
    void View::render()
    {
        //todo;
    }
    
    MEGA_PRIV_GETTER_SETTER(View, center, Vec)
    MEGA_PRIV_GETTER_SETTER(View, zoom, double)
    
    Vec View::viewToCanvas(Vec viewPoint)
    {
        //todo;
    }
    
    Vec View::viewToLayer(Vec viewPoint, Layer l)
    {
        //todo;
    }
}