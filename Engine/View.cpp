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
        
        GLuint segmentSizeGoal = max(tilew, tileh), segmentSize = 2;
        while (segmentSize < segmentSizeGoal)
            segmentSize <<= 1;

        if (segmentSize != $.mappingTextureSegmentSize) {
            $.mappingTextureSegmentSize = segmentSize;

            glActiveTexture(GL_TEXTURE0 + MAPPING_TU);
            glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R16, segmentSize*2, segmentSize*2, layers.size(), 
                         0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

            MEGA_ASSERT_GL_NO_ERROR;
            $.updateMappings();
        }
        
        glUniform2f($.uniforms.tileCount, $.viewTileCount[0], $.viewTileCount[1]);
        glUniform2f($.uniforms.invTileCount, 1.0/$.viewTileCount[0], 1.0/$.viewTileCount[1]);
        glUniform2f($.uniforms.tilePhase, 0.5*($.viewTileCount[0] - 1.0), 0.5*($.viewTileCount[1] - 1.0));
        glUniform1f($.uniforms.mappingTextureScale, 0.5/$.mappingTextureSegmentSize);
    }
    
    // this shouldn't exist in final implementation
    void Priv<View>::loadAllTiles()
    {
        auto tiles = $.canvas.tiles();
        std::size_t tileCount = $.tilesTextureCount = tiles.size();
        std::size_t tileSize = $.canvas.tileSize();
        
        glActiveTexture(GL_TEXTURE0 + TILES_TU);        
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, tileSize, tileSize, tileCount+1,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        
        // Tile 0 is the empty tile; all transparent
        std::unique_ptr<std::uint8_t[]> zeroes(new std::uint8_t[tileSize*tileSize*4]());
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0,
                        0, 0, 0, 
                        tileSize, tileSize, 1, 
                        GL_RGBA, GL_UNSIGNED_BYTE, zeroes.get());
        
        // Tiles 1 thru tileCount
        for (std::size_t i = 1; i <= tileCount; ++i) {
            auto tile = tiles[i-1];
            assert(tile.size() == tileSize*tileSize*4);
            glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0,
                            0, 0, i,
                            tileSize, tileSize, 1,
                            GL_RGBA, GL_UNSIGNED_BYTE, tile.data());
        }
        MEGA_ASSERT_GL_NO_ERROR;
    }
    
    namespace {
        void uploadSegment(ptrdiff_t x, ptrdiff_t y, size_t layeri, size_t segmentSize, uint16_t *segments)
        {
            glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0,
                            segmentSize*(x & 1), segmentSize*(y & 1), layeri,
                            segmentSize, segmentSize, 1,
                            GL_RED, GL_UNSIGNED_SHORT, 
                            reinterpret_cast<const GLvoid*>(segments));
        }
    }
        
    void Priv<View>::updateMappings()
    {
        using namespace std;
        static_assert(is_same<Layer::tile_t, uint16_t>::value, "you changed Layer::tile_t, gotta fix updateMappings");
        GLuint segmentSize = $.mappingTextureSegmentSize;

        if (segmentSize == 0)
            return;
        
        auto canvas = $.canvas;
        auto layers = canvas.layers();
        GLuint segmentArea = segmentSize*segmentSize;
        //fixme use pixel unpack buffer
        unique_ptr<uint16_t[]> segments(new uint16_t[4*segmentArea]);

        size_t layeri = 0;
        for (Layer layer : layers) {
            Vec layerCenter = ($.center - layer.origin()) * layer.parallax() + $.pixelAlign/$.zoom;
            Vec centerSegmentWithFrac = layerCenter/double(segmentSize * (canvas.tileSize() - 1));
            Vec centerSegment = centerSegmentWithFrac.round();
            ptrdiff_t segmentx = ptrdiff_t(centerSegment.x);
            ptrdiff_t segmenty = ptrdiff_t(centerSegment.y);
            layer.getSegment(segmentx - 1, segmenty - 1, &segments[0*segmentArea], segmentSize);
            layer.getSegment(segmentx,     segmenty - 1, &segments[1*segmentArea], segmentSize);
            layer.getSegment(segmentx - 1, segmenty,     &segments[2*segmentArea], segmentSize);
            layer.getSegment(segmentx,     segmenty,     &segments[3*segmentArea], segmentSize);
            
            glActiveTexture(GL_TEXTURE0 + MAPPING_TU);
            uploadSegment(segmentx - 1, segmenty - 1, layeri, segmentSize, &segments[0*segmentArea]);
            uploadSegment(segmentx,     segmenty - 1, layeri, segmentSize, &segments[1*segmentArea]);
            uploadSegment(segmentx - 1, segmenty,     layeri, segmentSize, &segments[2*segmentArea]);
            uploadSegment(segmentx,     segmenty,     layeri, segmentSize, &segments[3*segmentArea]);
            ++layeri;
        }
        MEGA_ASSERT_GL_NO_ERROR;
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

    bool View::prepare(std::string *outError)
    {
        assert(!$.prepared);
        llvm::raw_string_ostream errors(*outError);
        
        $.prepared = true;
        glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
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

        $.loadAllTiles(); //FIXME load on demand
        
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
            $.updateMappings();
            MEGA_ASSERT_GL_NO_ERROR;
        }
    }
    
    void View::moveCenter(Vec c)
    {
        $.center += c;
        if ($.good) {
            $.updateCenter(); //fixme progressive update
            $.updateMappings();
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