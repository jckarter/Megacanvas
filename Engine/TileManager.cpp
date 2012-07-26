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
#include <limits>
#include <llvm/ADT/ArrayRef.h>

namespace Mega {
    using namespace std;
    using namespace llvm;
    
    static constexpr size_t TEXTURE_SIZE = 2048;
    
    static constexpr Layer::tile_t NO_TILE = Layer::tile_t(-1);
    
    struct TileLayer {
        Rect readyRect = {0.0, 0.0, 0.0, 0.0};
        ptrdiff_t baseSegment[2] = {0, 0};
        Layer::SegmentRef segments[4];
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
        
        size_t tileSize, segmentSize, textureTileCount;
        
        Priv(Canvas c);
        
        void prepareTexture();
        ArrayRef<uint8_t> tile(size_t i);
        
        void loadTilesInRect(Rect r);
        void loadSegments(TileLayer &tl, Layer l, ptrdiff_t x, ptrdiff_t y);
        
        void uploadTile(TileLayer &tl, ptrdiff_t x, ptrdiff_t y, size_t layer);
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
    segmentSize(TEXTURE_SIZE >> (c.tileLogSize() + 1)),
    textureTileCount(segmentSize*segmentSize*4)
    {
        // nb: must be constructed with a valid GL context available
        $.texture.gen();
        $.pixelBuffers.gen();
        $.prepareTexture();
        
        for (size_t i = 0, end = c.layers().size(); i < end; ++i) {
            TileLayer &tl = $.tileLayers[i];
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
            return makeArrayRef($.zeroTile.get(), $.canvas.tileByteSize());
        MappedFile &file = $.tileCache[i-1];
        if (!file) {
            string error;
            file = $.canvas.loadTile(i, &error);
            assert(file);
        }
        file.willNeed();
        return file.data;
    }
    
    void Priv<TileManager>::loadTilesInRect(Rect r)
    {
        size_t tileSize = $.canvas.tileSize();
        auto layers = $.canvas.layers();

        for (size_t i = 0, end = layers.size(); i < end; ++i) {
            Layer l = layers[i];
            TileLayer &tl = $.tileLayers[i];
            
            Vec center = 0.5*(r.hi + r.lo), radius = 0.5*(r.hi - r.lo);
            Vec layerCenter = (center - l.origin()) * l.parallax();
            
            if (tl.readyRect.contains(layerCenter - radius)
                && tl.readyRect.contains(layerCenter + radius))
                continue;

            Vec loTile = ((layerCenter - radius)/tileSize).floor();
            Vec hiTile = ((layerCenter + radius)/tileSize).ceil();
            
            Vec loSeg = (loTile/$.segmentSize).floor();
            $.loadSegments(tl, l, loSeg.x, loSeg.y);
            
            for (ptrdiff_t y = loTile.y, yend = hiTile.y; y < yend; ++y)
                for (ptrdiff_t x = loTile.x, xend = hiTile.x; x < xend; ++x)
                    $.uploadTile(tl, x, y, i);
            
            tl.readyRect = Rect{loTile * tileSize, hiTile * tileSize};
        }
    }
    
    void Priv<TileManager>::loadSegments(TileLayer &tl, Layer l, ptrdiff_t x, ptrdiff_t y)
    {
        auto quadrant = [](ptrdiff_t x, ptrdiff_t y) { return (x&1) + (y&1)*2; };
        
        if (tl.baseSegment[0] != x || tl.baseSegment[1] != y) {
            tl.segments[quadrant(x  , y  )] = l.segment($.segmentSize, x,   y  );
            tl.segments[quadrant(x+1, y  )] = l.segment($.segmentSize, x+1, y  );
            tl.segments[quadrant(x  , y+1)] = l.segment($.segmentSize, x,   y+1);
            tl.segments[quadrant(x+1, y+1)] = l.segment($.segmentSize, x+1, y+1);
        }
    }
    
    void Priv<TileManager>::uploadTile(TileLayer &tl, ptrdiff_t x, ptrdiff_t y, size_t layer)
    {
        size_t xw = x & (segmentSize*2 - 1), yw = y & (segmentSize*2 - 1);
        size_t s = swizzle(xw, yw);
        size_t segi = s % (segmentSize*segmentSize);
        size_t seg = s/(segmentSize*segmentSize);
        assert(seg >= 0 && seg < 4);
        Layer::tile_t layerTile = tl.segments[seg][segi];
        Layer::tile_t &loadedTile = tl.tileMap[yw*segmentSize*2 + xw];
        
        if (loadedTile != layerTile) {
            loadedTile = layerTile;
            auto tile = $.tile(layerTile);
            
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, $.pixelBuffers.next());
            glBufferData(GL_PIXEL_UNPACK_BUFFER, tile.size(), tile.begin(), GL_STREAM_DRAW);
            MEGA_ASSERT_GL_NO_ERROR;
            
            glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0,
                            GLuint(xw * $.tileSize), GLuint(yw * $.tileSize), GLuint(layer),
                            GLuint($.tileSize), GLuint($.tileSize), 1,
                            GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            MEGA_ASSERT_GL_NO_ERROR;
            
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
            MEGA_ASSERT_GL_NO_ERROR;
        }
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
    
    void TileManager::require(Rect r)
    {
        $.requiredRect = r;
        $.loadTilesInRect(r);
    }
    
    void TileManager::centerHint(Vec center)
    {
        
    }
    
    void TileManager::viewportHint(Vec viewport, double zoom)
    {
        
    }
    
    bool TileManager::isTileReady(size_t tile)
    {
        for (size_t l = 0, end = $.canvas.layers().size(); l < end; ++l) {
            Layer::tile_t *tiles = $.tileLayers[l].tileMap.get();
            for (size_t i = 0, end = $.segmentSize*$.segmentSize*4; i < end; ++i)
                if (tiles[i] == tile)
                    return true;
        }
        return false;
    }
}