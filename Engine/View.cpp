//
//  View.cpp
//  Megacanvas
//
//  Created by Joe Groff on 6/29/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#include <cmath>

#include "Engine/View-priv.hpp"

namespace Mega {
    MEGA_PRIV_DTOR(View)

    Priv<View>::Priv(Canvas c)
    : canvas(c), prepared(false), good(false)
    {}

    Priv<View>::~Priv()
    {
        if (this->prepared) {
            this->deleteProgram();
            glDeleteTextures(1, &this->mappingTexture);
            glDeleteTextures(1, &this->tilesTexture);
            glDeleteVertexArrays(1, &this->meshArray);
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
        assert(this->good);
        
        double invTileSize = 1.0/double(this->canvas.tileSize() - 1);
        double invZoom = 1.0/this->zoom;
        auto layers = this->canvas.layers();
        GLuint tilew = this->viewTileCount[0] = GLuint(ceil(this->width*invZoom*invTileSize)) + 1;
        GLuint tileh = this->viewTileCount[1] = GLuint(ceil(this->height*invZoom*invTileSize)) + 1;
        
        GLuint tileCount = this->viewTileTotal = layers.size() * tilew * tileh;        

        unique_ptr<ViewVertex[]> vertices(new ViewVertex[4*tileCount]);
        unique_ptr<GLuint[]> elements(new GLuint[6*tileCount]);

        ViewVertex *vertex = vertices.get();
        GLuint *element = elements.get();
        GLuint tile = 0;
        for (GLuint layer = 0; layer < layers.size(); ++layer)
            for (GLuint y = 0; y < tileh; ++y)
                for (GLuint x = 0; x < tilew; ++x, tile += 4) {
                    initVertex(vertex++, x, y, layer, layers[layer], 0.0f, 0.0);
                    initVertex(vertex++, x, y, layer, layers[layer], 1.0f, 0.0);
                    initVertex(vertex++, x, y, layer, layers[layer], 0.0f, 1.0);
                    initVertex(vertex++, x, y, layer, layers[layer], 1.0f, 1.0);
                    
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
        
        glBindVertexArray(this->meshArray);
        bindVertexAttributes<ViewVertex>(this->program);
        
        MEGA_ASSERT_GL_NO_ERROR;
        
        GLuint segmentSizeGoal = max(tilew, tileh), segmentSize = 2;
        while (segmentSize < segmentSizeGoal)
            segmentSize <<= 1;

        if (segmentSize != this->mappingTextureSegmentSize) {
            this->mappingTextureSegmentSize = segmentSize;

            glActiveTexture(GL_TEXTURE0 + MAPPING_TU);
            glBindTexture(GL_TEXTURE_2D_ARRAY, this->mappingTexture);
            glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R16, segmentSize*2, segmentSize*2, layers.size(), 
                         0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

            MEGA_ASSERT_GL_NO_ERROR;
            this->updateMappings();
        }
        
        glUniform2f(this->uniforms.tileCount, this->viewTileCount[0], this->viewTileCount[1]);
        glUniform1f(this->uniforms.mappingTextureScale, 0.5/this->mappingTextureSegmentSize);
    }
    
    void Priv<View>::loadAllTiles()
    {
        auto tiles = this->canvas.tiles();
        size_t tileCount = this->tilesTextureCount = tiles.size();
        size_t tileSize = this->canvas.tileSize();
        
        glActiveTexture(GL_TEXTURE0 + TILES_TU);
        glBindTexture(GL_TEXTURE_2D_ARRAY, this->tilesTexture);
        
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, tileSize, tileSize, tileCount,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        
        for (size_t i = 0; i < tileCount; ++i) {
            auto tile = tiles[i];
            assert(tile.size() == tileSize*tileSize*4);
            glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0,
                            0, 0, i,
                            tileSize, tileSize, 1,
                            GL_RGBA, GL_UNSIGNED_BYTE, tile.data());
        }
        MEGA_ASSERT_GL_NO_ERROR;
    }
    
    void Priv<View>::updateMappings()
    {
        //todo;
    }
    
    void Priv<View>::updateShaderParams()
    {
        glUniform1f(this->uniforms.tileSize, this->canvas.tileSize());
        glUniform1i(this->uniforms.mappingTexture, MAPPING_TU);
        glUniform1i(this->uniforms.tilesTexture, TILES_TU);
        MEGA_ASSERT_GL_NO_ERROR;
    }
    
    void Priv<View>::updateCenter()
    {
        glUniform2f(this->uniforms.center, this->center.x, this->center.y);
        MEGA_ASSERT_GL_NO_ERROR;
    }
    
    void Priv<View>::updateViewport()
    {
        glViewport(0, 0, this->width, this->height);
        glUniform2f(this->uniforms.viewport, this->width, this->height);
        MEGA_ASSERT_GL_NO_ERROR;
    }
    
    bool Priv<View>::createProgram(GLuint *outFrag, GLuint *outVert, GLuint *outProg, std::string *outError)
    {
        using namespace llvm;
        using namespace std;
        OwningPtr<MemoryBuffer> fragSource, vertSource;
        string basename = shaderPath;
        basename += "/megacanvas";
        if (!loadProgramSource(basename, &vertSource, &fragSource, outError))
            return false;
        llvm::SmallString<16> log;
        if (!compileProgram(vertSource->getBuffer(), 
                            fragSource->getBuffer(), 
                            outVert, outFrag, outProg,
                            &log)) {
            *outError = "shader compile error:\n";
            *outError += log;
            return false;
        }
        
        glBindFragDataLocation(*outProg, 0, "color");
        
        if (!linkProgram(*outProg, &log)) {
            *outError = "shader link error:\n";
            *outError += log;
            destroyProgram(*outVert, *outFrag, *outProg);
            *outVert = *outFrag = *outProg = 0;
            return false;
        }
        
        return true;
    }

    void Priv<View>::deleteProgram()
    {
        destroyProgram(this->vertShader, this->fragShader, this->program);
        this->vertShader = this->fragShader = this->program = 0;
    }

    Owner<View> View::create(Canvas c)
    {
        return createOwner<View>(c);
    }

    bool View::prepare(std::string *outError)
    {
        assert(!that->prepared);
        llvm::raw_string_ostream errors(*outError);
        
        that->prepared = true;
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glGenBuffers(1, &that->meshBuffer);
        glGenBuffers(1, &that->eltBuffer);
        glGenVertexArrays(1, &that->meshArray);

        glGenTextures(1, &that->tilesTexture);
        glActiveTexture(GL_TEXTURE0 + TILES_TU);
        glBindTexture(GL_TEXTURE_2D_ARRAY, that->tilesTexture);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        glGenTextures(1, &that->mappingTexture);
        glActiveTexture(GL_TEXTURE0 + MAPPING_TU);
        glBindTexture(GL_TEXTURE_2D_ARRAY, that->mappingTexture);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);

        that->loadAllTiles(); //FIXME load on demand
        
        auto error = glGetError();
        if (error != GL_NO_ERROR) {
            errors << "OpenGL error " << error;
            errors.flush();
            return false;
        }

        if (!that->createProgram(&that->fragShader, &that->vertShader, &that->program, outError))
            return false;
        if (!getUniformLocations(that->program, &that->uniforms)) {
            errors << "Unable to initialize shader parameters";
            errors.flush();
            that->deleteProgram();
            return false;
        }
        
        glUseProgram(that->program);
        
        that->updateShaderParams();
        that->updateCenter();
        that->updateViewport();
        
        MEGA_ASSERT_GL_NO_ERROR;
        
        that->good = true;
        return true;
    }

    void View::resize(double width, double height)
    {
        that->width = width;
        that->height = height;
        if (that->good) {
            //viewport
            that->updateViewport();
            that->updateMesh();

            MEGA_ASSERT_GL_NO_ERROR;
        }
    }

    void View::render()
    {
        assert(that->good);
        //todo;
        MEGA_ASSERT_GL_NO_ERROR;
    }

    MEGA_PRIV_GETTER_SETTER(View, center, Vec)
    MEGA_PRIV_GETTER(View, zoom, double)

    void View::zoom(double x)
    {
        that->zoom = x;
        if (that->good) {
            that->updateMesh();
            MEGA_ASSERT_GL_NO_ERROR;
        }
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