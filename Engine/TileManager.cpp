//
//  TileManager.cpp
//  Megacanvas
//
//  Created by Joe Groff on 7/24/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#include "Engine/TileManager.hpp"
#include "Engine/Canvas.hpp"
#include "Engine/Layer.hpp"
#include "Engine/Util/GLMeta.hpp"
#include "Engine/Util/MappedFile.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <limits>
#include <llvm/ADT/ArrayRef.h>
#include <mutex>
#include <thread>

#define MEGA_TILE_MANAGER_STATS

namespace Mega {
    using namespace std;
    using namespace llvm;
    
    static constexpr size_t TEXTURE_SIZE = 4096;
    
    static constexpr Layer::tile_t NO_TILE = Layer::tile_t(-1);
    
    struct TileLayer {
        Rect readyRect = {0.0, 0.0, 0.0, 0.0};
        unique_ptr<Layer::tile_t[]> tileMap;
    };
    
    template<>
    struct Priv<TileManager> {
        Canvas canvas;
        GLTexture texture;
        FlipFlop<GLBuffer> pixelBuffers;
        Rect requiredRect;
        
        unique_ptr<uint8_t[]> zeroTile;
        unique_ptr<MappedFile[]> tileCache;
        unique_ptr<TileLayer[]> tileLayers;
        
        size_t tileSize, textureTileSize, textureTileCount;
        
        thread prefetcher;
        bool runPrefetcher;
        mutex lockPrefetcher;
        condition_variable cvPrefetcher;
        atomic<size_t> viewportAge;
        
        Priv(Canvas c);
        ~Priv();
        
        void prepareTexture();
        ArrayRef<uint8_t> tile(size_t i);
        
        void loadTilesInView(Vec center, Vec viewport);        
        bool uploadTile(TileLayer &tl, Layer l,
                        ptrdiff_t x, ptrdiff_t y, size_t layer);
        
        ArrayRef<uint8_t> zeroTileRef() {
            return {zeroTile.get(), $.canvas.tileByteSize()};
        }
        
        MutableArrayRef<MappedFile> tileCacheRef() {
            return {tileCache.get(), $.canvas.tileCount()};
        }
        
        MutableArrayRef<TileLayer> tileLayersRef() {
            return {tileLayers.get(), $.canvas.layers().size()};
        }
        
        MutableArrayRef<Layer::tile_t> tileMapRef(size_t layer) {
            return {tileLayersRef()[layer].tileMap.get(), $.textureTileCount};
        }
        
        void prefetchThread(gl_context_t mainContext);
    };
    MEGA_PRIV_DTOR(TileManager)
        
    Priv<TileManager>::Priv(Canvas c)
    :
    canvas(c),
    requiredRect{0.0, 0.0, 0.0, 0.0},
    zeroTile(new uint8_t[c.tileByteSize()]()),
    tileCache(new MappedFile[c.tileCount()]()),
    tileLayers(new TileLayer[c.layers().size()]()),
    tileSize(c.tileSize()),
    textureTileSize(TEXTURE_SIZE >> c.tileLogSize()),
    textureTileCount(textureTileSize*textureTileSize),
    runPrefetcher(true),
    viewportAge(0)
    {
        // nb: must be constructed with a valid GL context available
        $.texture.gen();
        $.pixelBuffers.gen();
        $.prepareTexture();
        
        for (TileLayer &tl : $.tileLayersRef()) {
            tl.tileMap.reset(new Layer::tile_t[$.textureTileCount]);
            fill(&tl.tileMap[0], &tl.tileMap[$.textureTileCount], NO_TILE);
        }
        
        prefetcher = thread([](Priv *that, gl_context_t mainContext) {
            that->prefetchThread(mainContext);
        }, this, currentGLContext());
    }
    
    Priv<TileManager>::~Priv()
    {
        {
            unique_lock<mutex> lock($.lockPrefetcher);
            $.runPrefetcher = false;
            $.cvPrefetcher.notify_all();
        }
        $.prefetcher.join();
    }
    
    void Priv<TileManager>::prepareTexture()
    {
        assert($.texture);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, $.texture);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        MEGA_ASSERT_GL_NO_ERROR;
        
        assert($.canvas.layers().size() <= numeric_limits<GLsizei>::max());
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_SRGB8_ALPHA8, 
                     TEXTURE_SIZE, TEXTURE_SIZE, GLsizei($.canvas.layers().size()),
                     0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        MEGA_ASSERT_GL_NO_ERROR;
    }
    
    // fixme: need to be able to unmap stale tiles after threshold reached
    ArrayRef<uint8_t> Priv<TileManager>::tile(size_t i)
    {
        assert(i >= 0 && i <= $.canvas.tileCount());
        if (i == 0)
            return $.zeroTileRef();
        MappedFile &file = $.tileCacheRef()[i-1];
        if (!file) {
            string error;
            file = $.canvas.loadTile(i, &error);
            assert(file);
        }
        file.willNeed();
        return file.data;
    }
    
    void Priv<TileManager>::loadTilesInView(Vec center, Vec viewport)
    {
#ifdef MEGA_TILE_MANAGER_STATS
        auto begun = chrono::high_resolution_clock::now();
        size_t uploaded = 0;
#endif
        size_t tileSize = $.tileSize;
        auto layers = $.canvas.layers();
        Vec radius = 0.5*viewport;
        
        if (viewport.x > (TEXTURE_SIZE - $.tileSize)
            || viewport.y > (TEXTURE_SIZE - $.tileSize))
            errs() << "warning: viewport dimensions " << viewport.x << ","
            << viewport.y << " too large for texture size\n";

        for (size_t i = 0, end = layers.size(); i < end; ++i) {
            Layer l = layers[i];
            TileLayer &tl = $.tileLayersRef()[i];
            
            Vec layerCenter = (center - l.origin()) * l.parallax();
            
            if (tl.readyRect.contains(layerCenter - radius)
                && tl.readyRect.contains(layerCenter + radius))
                continue;

            Vec loTile = ((layerCenter - radius)/tileSize).floor();
            Vec hiTile = ((layerCenter + radius)/tileSize).ceil();
            
            for (ptrdiff_t y = loTile.y, yend = hiTile.y; y < yend; ++y)
                for (ptrdiff_t x = loTile.x, xend = hiTile.x; x < xend; ++x) {
                    bool didUpload = $.uploadTile(tl, l, x, y, i);
#ifdef MEGA_TILE_MANAGER_STATS
                    if (didUpload) ++uploaded;
#endif
                }
            
            tl.readyRect = Rect{loTile * tileSize, hiTile * tileSize};
        }

#ifdef MEGA_TILE_MANAGER_STATS
        auto ended = chrono::high_resolution_clock::now();
        errs() << "uploaded " << uploaded << " tiles in "
        << chrono::duration_cast<chrono::nanoseconds>(ended - begun).count() << " ns\n";
#endif
    }
    
    bool Priv<TileManager>::uploadTile(TileLayer &tl, Layer l,
                                       ptrdiff_t x, ptrdiff_t y, size_t layer)
    {
        size_t xw = x & ($.textureTileSize-1), yw = y & ($.textureTileSize-1);
        Layer::tile_t layerTile = l.segment(1, x, y)[0];
        Layer::tile_t &loadedTile = $.tileMapRef(layer)[yw*textureTileSize + xw];
        
        if (loadedTile != layerTile) {
            loadedTile = layerTile;
            auto tile = $.tile(layerTile);
            
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, $.pixelBuffers.next());
            glBufferData(GL_PIXEL_UNPACK_BUFFER, tile.size(), tile.begin(), GL_STREAM_DRAW);
            /*glBufferData(GL_PIXEL_UNPACK_BUFFER, tile.size(), nullptr, GL_STREAM_DRAW);
            void *buf = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
            assert(buf);
            memcpy(buf, tile.begin(), tile.size());
            glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);*/
            MEGA_ASSERT_GL_NO_ERROR;
            
            glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0,
                            GLuint(xw * $.tileSize), GLuint(yw * $.tileSize), GLuint(layer),
                            GLuint($.tileSize), GLuint($.tileSize), 1,
                            GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            MEGA_ASSERT_GL_NO_ERROR;
            
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
            MEGA_ASSERT_GL_NO_ERROR;
            
            return true;
        }
        return false;
    }
    
    void Priv<TileManager>::prefetchThread(gl_context_t mainContext)
    {
        char const *error;
        GLContext ourContext(makeSharedGLContext(mainContext, &error));
        
        if (!ourContext) {
            errs() << "unable to create shared GL context for prefetch thread: " << error << '\n';
            goto done;
        }
        
        setCurrentGLContext(ourContext);

        $$.bindState();
        
        errs() << "\nstart";
        {
            unique_lock<mutex> lock($.lockPrefetcher);
            size_t lastViewportAge = 0, viewportAge;
            while ($.runPrefetcher) {
                $.cvPrefetcher.wait(lock);
                
                while ((viewportAge = $.viewportAge.load()) > lastViewportAge) {
                    // do some prefetching
                    lastViewportAge = viewportAge;
                }
                errs() << "."; errs().flush();
            }
        }
        
    done:
        errs() << "end\n";
    }

    Owner<TileManager> TileManager::create(Canvas c)
    {
        return createOwner<TileManager>(c);
    }
    
    void TileManager::bindState()
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, $.texture);
    }
    
    MEGA_PRIV_GETTER(TileManager, texture, GLuint)
    
    size_t TileManager::textureSize()
    {
        return TEXTURE_SIZE;
    }
    
    void TileManager::require(Vec center, Vec viewport)
    {
        $.loadTilesInView(center, viewport);
    }
    
    bool TileManager::isTileReady(size_t tile)
    {
        for (size_t l = 0, end = $.canvas.layers().size(); l < end; ++l) {
            auto tileMap = $.tileMapRef(l);
            for (Layer::tile_t mappedTile : tileMap)
                if (mappedTile == tile)
                    return true;
        }
        return false;
    }
}