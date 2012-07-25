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

namespace Mega {
    using namespace std;
    using namespace llvm;
    
    static constexpr size_t TEXTURE_SIZE = 4096;
    
    template<>
    struct Priv<TileManager> {
        Canvas canvas;
        GLTexture texture;
        Rect requiredRect, readyRect;
        
        unique_ptr<MappedFile[]> tileCache; 
        
        Priv(Canvas c);
        
        void prepareTexture();
        ArrayRef<uint8_t> tile(size_t i);
    };
    MEGA_PRIV_DTOR(TileManager)
    
    Priv<TileManager>::Priv(Canvas c)
    :
    canvas(c),
    requiredRect(0.0, 0.0, 0.0, 0.0), readyRect(0.0, 0.0, 0.0, 0.0),
    tileCache(new MappedFile[c.tileCount()])
    {
        // nb: must be constructed with a valid GL context available
        texture.gen();
        $.prepareTexture();
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
    
    // fixme: need to be able to eject stale tiles after threshold reached
    ArrayRef<uint8_t> Priv<TileManager>::tile(size_t i)
    {
        assert(i >= 1 && i <= $.canvas.tileCount());
        MappedFile &file = $.tileCache[i-1];
        if (!file) {
            string error;
            file = $.canvas.loadTile(i, &error);
            assert(file);
        }
        file.willNeed();
        return file.data;
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
    }
    
    void TileManager::centerHint(Vec center)
    {
        
    }
    
    void TileManager::viewportHint(Vec viewport, double zoom)
    {
        
    }
    
    bool TileManager::isTileReady(size_t tile)
    {
        return false;
    }
}