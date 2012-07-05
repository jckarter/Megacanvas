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
    
    namespace {
        void initVertex(ViewVertex *vertex, GLuint x, GLuint y, GLuint layeri, Layer layer,
                        float cornerX, float cornerY)
        {
            vertex->tileCoord[0] = float(x);
            vertex->tileCoord[1] = float(y);
            vertex->tileCoord[2] = float(layeri);
            vertex->layerParallax[0] = layer.parallax().x;
            vertex->layerParallax[1] = layer.parallax().y;
            vertex->layerOrigin[0] = layer.origin().x;
            vertex->layerOrigin[1] = layer.origin().y;
            vertex->tileCorner[0] = cornerX;
            vertex->tileCorner[1] = cornerY;            
        }
    }

    void Priv<View>::updateMesh()
    {
        using namespace std;
        double invTileSize = 1.0/double(this->canvas.tileSize() - 1);
        double invZoom = 1.0/this->zoom;
        auto layers = this->canvas.layers();
        GLuint tilew = GLuint(ceil(this->width*invZoom*invTileSize)) + 1, tileh = GLuint(ceil(this->height*invZoom*invTileSize)) + 1;
        GLuint tileCount = this->viewTileCount = layers.size() * tilew * tileh;        

        unique_ptr<ViewVertex[]> vertices(new ViewVertex[4*tileCount]);
        unique_ptr<GLuint> elements(new GLuint[6*tileCount]);

        ViewVertex *vertex = vertices.get();
        GLuint *element = elements.get();
        GLuint tile = 0;
        for (GLuint layer = 0; layer < layers.size(); ++layer)
            for (GLuint y = 0; y < tileh; ++y)
                for (GLuint x = 0; x < tilew; ++x, tile += 4) {
                    initVertex(vertex++, x, y, layer, layers[layer], -1.0f, -1.0);
                    initVertex(vertex++, x, y, layer, layers[layer],  1.0f, -1.0);
                    initVertex(vertex++, x, y, layer, layers[layer], -1.0f,  1.0);
                    initVertex(vertex++, x, y, layer, layers[layer],  1.0f,  1.0);
                    
                    *element++ = tile  ;
                    *element++ = tile+1;
                    *element++ = tile+2;
                    *element++ = tile+1;
                    *element++ = tile+3;
                    *element++ = tile+2;
                }
        
        glBindBuffer(GL_ARRAY_BUFFER, this->meshBuffer);
        glBufferData(GL_ARRAY_BUFFER, 4*tileCount*sizeof(ViewVertex),
                     reinterpret_cast<const GLvoid*>(vertices.get()), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->eltBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6*tileCount*sizeof(GLuint),
                     reinterpret_cast<const GLvoid*>(elements.get()), GL_STATIC_DRAW);
    }
    
    void Priv<View>::createProgram(GLuint *outFrag, GLuint *outVert, GLuint *outProg)
    {
        //todo;
    }

    void Priv<View>::deleteProgram()
    {
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