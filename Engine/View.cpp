//
//  View.cpp
//  Megacanvas
//
//  Created by Joe Groff on 6/29/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#include <cmath>

#include "Engine/Util/GL.h"
#include "Engine/Canvas.hpp"
#include "Engine/Layer.hpp"
#include "Engine/View-priv.hpp"

namespace Mega {
    MEGA_PRIV_DTOR(View)

    Priv<View>::Priv(Canvas c)
    : canvas(c)
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
        double invTileSize = 1.0/double(this->canvas.tileSize() - 1);
        double invZoom = 1.0/this->zoom;
        size_t tilew = size_t(ceil(this->width*invZoom*invTileSize)) + 1, tileh = size_t(ceil(this->height*invZoom*invTileSize)) + 1;
        GLuint tileCount = this->viewTileCount = tilew * tileh;
        size_t vertexCount = this->canvas.layers().size() * tileCount;

        //todo;
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
        assert(that->prepared);
        that->width = width;
        that->height = height;
        //viewport
        that->updateMesh();
    }

    void View::render()
    {
        assert(that->prepared);
        //todo;
    }

    MEGA_PRIV_GETTER_SETTER(View, center, Vec)
    MEGA_PRIV_GETTER(View, zoom, double)

    void View::zoom(double x)
    {
        that->zoom = x;
        if (that->prepared)
            that->updateMesh();
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