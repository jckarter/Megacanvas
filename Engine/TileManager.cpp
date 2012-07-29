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
#include <deque>
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
        
        unique_ptr<uint8_t[]> zeroTile;
        unique_ptr<MappedFile[]> tileCache;
        unique_ptr<TileLayer[]> tileLayers;
        
        size_t tileSize, textureTileSize, textureTileCount;
        
        Priv(Canvas c);
        
        void prepareTexture();
        ArrayRef<uint8_t> tile(size_t i);
        
        bool loadTilesInView(Vec center, Vec viewport);
        
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
    };
    MEGA_PRIV_DTOR(TileManager)
        
    Priv<TileManager>::Priv(Canvas c)
    :
    canvas(c),
    zeroTile(new uint8_t[c.tileByteSize()]()),
    tileCache(new MappedFile[c.tileCount()]()),
    tileLayers(new TileLayer[c.layers().size()]()),
    tileSize(c.tileSize()),
    textureTileSize(TEXTURE_SIZE >> c.tileLogSize()),
    textureTileCount(textureTileSize*textureTileSize)
    {
        // nb: must be constructed with a valid GL context available
        $.texture.gen();
        $.pixelBuffers.gen();
        $.prepareTexture();
        
        for (TileLayer &tl : $.tileLayersRef()) {
            tl.tileMap.reset(new Layer::tile_t[$.textureTileCount]);
            fill(&tl.tileMap[0], &tl.tileMap[$.textureTileCount], NO_TILE);
        }
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
    
    bool Priv<TileManager>::loadTilesInView(Vec center, Vec viewport)
    {
#ifdef MEGA_TILE_MANAGER_STATS
        auto begun = chrono::high_resolution_clock::now();
#endif
        size_t tileSize = $.tileSize;
        auto layers = $.canvas.layers();
        Vec radius = 0.5*viewport;
        
        struct Upload { ArrayRef<uint8_t> tile; size_t xw; size_t yw; size_t layer; };
        SmallVector<Upload, 16> uploads;
        
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
                    size_t xw = x & ($.textureTileSize-1), yw = y & ($.textureTileSize-1);
                    Layer::tile_t layerTile = l.segment(1, x, y)[0];
                    Layer::tile_t &loadedTile = $.tileMapRef(i)[yw*textureTileSize + xw];
                    
                    if (loadedTile != layerTile) {
                        loadedTile = layerTile;
                        uploads.push_back(Upload{$.tile(layerTile), xw, yw, i});
                    }
                }
            
            tl.readyRect = Rect{loTile * tileSize, hiTile * tileSize};
        }

        for (Upload &upload : uploads) {
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, $.pixelBuffers.next());
            glBufferData(GL_PIXEL_UNPACK_BUFFER,
                         upload.tile.size(), upload.tile.begin(),
                         GL_STREAM_DRAW);
            /*glBufferData(GL_PIXEL_UNPACK_BUFFER, tile.size(), nullptr, GL_STREAM_DRAW);
             void *buf = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
             assert(buf);
             memcpy(buf, tile.begin(), tile.size());
             glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);*/
            MEGA_ASSERT_GL_NO_ERROR;
            
            glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0,
                            GLuint(upload.xw * $.tileSize),
                            GLuint(upload.yw * $.tileSize),
                            GLuint(upload.layer),
                            GLuint($.tileSize), GLuint($.tileSize), 1,
                            GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            MEGA_ASSERT_GL_NO_ERROR;
        }
        
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        MEGA_ASSERT_GL_NO_ERROR;

#ifdef MEGA_TILE_MANAGER_STATS
        auto ended = chrono::high_resolution_clock::now();
        errs() << "uploaded " << uploads.size() << " tiles in "
        << chrono::duration_cast<chrono::nanoseconds>(ended - begun).count() << " ns\n";
#endif
        
        return true;
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
    
    bool TileManager::require(Vec center, Vec viewport)
    {
        return $.loadTilesInView(center, viewport);
    }
    
    bool TileManager::prefetch() {
        
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