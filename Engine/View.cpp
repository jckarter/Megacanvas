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
        if ($.prepared) {
            $.deleteProgram();
            glDeleteTextures(1, &$.mappingTexture);
            glDeleteTextures(1, &$.tilesTexture);
            glDeleteVertexArrays(1, &$.meshArray);
            glDeleteBuffers(1, &$.eltBuffer);
            glDeleteBuffers(1, &$.meshBuffer);
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
    
    namespace {
        size_t swizzle(std::size_t x, std::size_t y)
        {
            assert(x <= 0xFFFF && y <= 0xFFFF);
            static const unsigned int B[] = {0x55555555, 0x33333333, 0x0F0F0F0F, 0x00FF00FF};
            static const unsigned int S[] = {1, 2, 4, 8};
            
            x = (x | (x << S[3])) & B[3];
            x = (x | (x << S[2])) & B[2];
            x = (x | (x << S[1])) & B[1];
            x = (x | (x << S[0])) & B[0];
            
            y = (y | (y << S[3])) & B[3];
            y = (y | (y << S[2])) & B[2];
            y = (y | (y << S[1])) & B[1];
            y = (y | (y << S[0])) & B[0];
            
            return x | (y << 1);
        }
    }

    void Priv<View>::updateMesh()
    {
        using namespace std;
        assert($.good);
        
        double invTileSize = 1.0/double($.canvas.tileSize() - 1);
        double invZoom = 1.0/$.zoom;
        auto layers = $.canvas.layers();
        GLuint tilew = $.viewTileCount[0] = GLuint(ceil($.width*invZoom*invTileSize)) + 1;
        GLuint tileh = $.viewTileCount[1] = GLuint(ceil($.height*invZoom*invTileSize)) + 1;
        
        GLuint tileCount = $.viewTileTotal = layers.size() * tilew * tileh;        

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
        
        glBufferData(GL_ARRAY_BUFFER, 4*tileCount*sizeof(ViewVertex),
                     reinterpret_cast<const GLvoid*>(vertices.get()), GL_STATIC_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6*tileCount*sizeof(GLuint),
                     reinterpret_cast<const GLvoid*>(elements.get()), GL_STATIC_DRAW);
        
        bindVertexAttributes<ViewVertex>($.program);
        
        MEGA_ASSERT_GL_NO_ERROR;
        
        GLuint quadrantSizeGoal = max(tilew, tileh), quadrantSize = 1;
        while (quadrantSize < quadrantSizeGoal)
            quadrantSize <<= 1;
        GLuint mappingSize = quadrantSize << 1;

        if (mappingSize != $.mappingTextureSize) {
            $.mappingTextureSize = mappingSize;

            size_t layerCount = layers.size();
            size_t layerSize = mappingSize * mappingSize;
            size_t bufferSize = layerCount * layerSize;
            
            assert(bufferSize < 65536); // FIXME
            unique_ptr<uint16_t[]> mappingBuffer(new uint16_t[bufferSize]);
            uint16_t *mapping = mappingBuffer.get();
            for (size_t l = 0; l < bufferSize; l += layerSize)
                for (size_t y = 0; y < mappingSize; ++y)
                    for (size_t x = 0; x < mappingSize; ++x)
                        *mapping++ = l + swizzle(x, y);
            
            glActiveTexture(GL_TEXTURE0 + MAPPING_TU);
            glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R16, mappingSize, mappingSize, layerCount, 
                         0, GL_RED, GL_UNSIGNED_SHORT, mappingBuffer.get());

            MEGA_ASSERT_GL_NO_ERROR;
            
            std::size_t tileSize = $.canvas.tileSize();
            
            glActiveTexture(GL_TEXTURE0 + TILES_TU);        
            glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_SRGB8_ALPHA8,
                         tileSize, tileSize, bufferSize,
                         0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            
            MEGA_ASSERT_GL_NO_ERROR;

            $.loadVisibleTiles();
        }
        
        glUniform2f($.uniforms.tileCount, $.viewTileCount[0], $.viewTileCount[1]);
        glUniform2f($.uniforms.invTileCount, 1.0/$.viewTileCount[0], 1.0/$.viewTileCount[1]);
        glUniform2f($.uniforms.tilePhase, 0.5*($.viewTileCount[0] - 1.0), 0.5*($.viewTileCount[1] - 1.0));
        glUniform1f($.uniforms.mappingTextureScale, 1.0/$.mappingTextureSize);
    }
    
    void Priv<View>::loadVisibleTiles()
    {
        //fixme
    }
    
    bool Priv<View>::isTileLoaded(std::size_t tile)
    {
        return false;
    }
    
    void Priv<View>::updateShaderParams()
    {
        float tileSize = $.canvas.tileSize();
        glUniform1f($.uniforms.tileTrimSize, tileSize - 1.0f);
        glUniform1f($.uniforms.invTileTrimSize, 1.0f/(tileSize - 1.0f));
        glUniform1f($.uniforms.tileTexLo, 0.5f/tileSize);
        glUniform1f($.uniforms.tileTexSize, (tileSize - 1.0f)/tileSize);
        glUniform1i($.uniforms.mappingTexture, MAPPING_TU);
        glUniform1i($.uniforms.tilesTexture, TILES_TU);
        MEGA_ASSERT_GL_NO_ERROR;
    }
    
    void Priv<View>::updateCenter()
    {
        // fixme layer-specific rounding
        Vec center = ($.center * $.zoom).round() / $.zoom;
        glUniform2f($.uniforms.center, center.x, center.y);
        MEGA_ASSERT_GL_NO_ERROR;
    }
    
    void Priv<View>::updateViewport()
    {
        glViewport(0, 0, $.width, $.height);
        $.updateZoom();
    }
    
    void Priv<View>::updateZoom()
    {
        glUniform2f($.uniforms.viewportScale, 2.0*$.zoom/$.width, 2.0*$.zoom/$.height);
        Vec pixelAlign = $.pixelAlign/$.zoom;
        glUniform2f($.uniforms.pixelAlign, pixelAlign.x, pixelAlign.y);
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
        destroyProgram($.vertShader, $.fragShader, $.program);
        $.vertShader = $.fragShader = $.program = 0;
    }

    Owner<View> View::create(Canvas c)
    {
        return createOwner<View>(c);
    }
    
    namespace {
        bool defaultFramebufferIsSRGB()
        {
            GLint param;
            glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &param);
            GLenum attachment = param == 0 ? GL_BACK_LEFT : GL_COLOR_ATTACHMENT0;
            glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, attachment, GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING, &param);
            MEGA_ASSERT_GL_NO_ERROR;
            return param == GL_SRGB;
        }
    }

    bool View::prepare(std::string *outError)
    {
        assert(!$.prepared);
        llvm::raw_string_ostream errors(*outError);
        
        $.prepared = true;
        glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
        
        glEnable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glEnable(GL_FRAMEBUFFER_SRGB);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        assert(defaultFramebufferIsSRGB());
        
        glGenBuffers(1, &$.meshBuffer);
        glGenBuffers(1, &$.eltBuffer);
        glGenVertexArrays(1, &$.meshArray);

        glGenTextures(1, &$.tilesTexture);
        glActiveTexture(GL_TEXTURE0 + TILES_TU);
        glBindTexture(GL_TEXTURE_2D_ARRAY, $.tilesTexture);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        glGenTextures(1, &$.mappingTexture);
        glActiveTexture(GL_TEXTURE0 + MAPPING_TU);
        glBindTexture(GL_TEXTURE_2D_ARRAY, $.mappingTexture);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);

        auto error = glGetError();
        if (error != GL_NO_ERROR) {
            errors << "OpenGL error " << error;
            errors.flush();
            return false;
        }

        if (!$.createProgram(&$.fragShader, &$.vertShader, &$.program, outError))
            return false;
        if (!getUniformLocations($.program, &$.uniforms)) {
            errors << "Unable to initialize shader parameters";
            errors.flush();
            $.deleteProgram();
            return false;
        }
        
        this->bindState();        
        $.updateShaderParams();
        $.updateCenter();        
        
        MEGA_ASSERT_GL_NO_ERROR;
        
        $.good = true;
        return true;
    }
    
    void View::bindState()
    {
        glUseProgram($.program);
        glBindVertexArray($.meshArray);
        glBindBuffer(GL_ARRAY_BUFFER, $.meshBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, $.eltBuffer);
        glActiveTexture(GL_TEXTURE0 + TILES_TU);
        glBindTexture(GL_TEXTURE_2D_ARRAY, $.tilesTexture);
        glActiveTexture(GL_TEXTURE0 + MAPPING_TU);
        glBindTexture(GL_TEXTURE_2D_ARRAY, $.mappingTexture);
    }

    void View::resize(double width, double height)
    {
        $.width = width;
        $.height = height;
        Vec align = 0.5*(1.0+Vec(width, height));
        $.pixelAlign = align - align.floor();
        if ($.good) {
            $.updateViewport();
            $.updateMesh();
            MEGA_ASSERT_GL_NO_ERROR;
        }
    }
    
    void View::render()
    {
        assert($.good);
        
        glClear(GL_COLOR_BUFFER_BIT); //FIXME checkered background
        
        glDrawElements(GL_TRIANGLES, 6*$.viewTileTotal, GL_UNSIGNED_INT, nullptr);
        MEGA_ASSERT_GL_NO_ERROR;
    }

    MEGA_PRIV_GETTER(View, center, Vec)
    
    void View::center(Vec c)
    {
        $.center = c;
        if ($.good) {
            $.updateCenter(); //fixme progressive update
            $.loadVisibleTiles();
            MEGA_ASSERT_GL_NO_ERROR;
        }
    }
    
    void View::moveCenter(Vec c)
    {
        $.center += c;
        if ($.good) {
            $.updateCenter(); //fixme progressive update
            $.loadVisibleTiles();
            MEGA_ASSERT_GL_NO_ERROR;            
        }
    }
    
    MEGA_PRIV_GETTER(View, zoom, double)
    
    void View::zoom(double x)
    {
        $.zoom = std::max(x, 1.0);
        if ($.good) {
            $.updateMesh();
            $.updateCenter();
            $.updateZoom();
            MEGA_ASSERT_GL_NO_ERROR;
        }
    }
    
    void View::moveZoom(double x)
    {
        this->zoom($.zoom + x);
    }

    Vec View::viewToCanvas(Vec viewPoint)
    {
        //todo;
    }

    Vec View::viewToLayer(Vec viewPoint, Layer l)
    {
        //todo;
    }
    
    void View::syncTextureStreaming()
    {
        
    }
}