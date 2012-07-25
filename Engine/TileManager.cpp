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

namespace Mega {
    static constexpr std::size_t TEXTURE_SIZE = 4096;
    
    template<>
    struct Priv<TileManager> {
        Canvas canvas;
        GLTexture texture;
        
        std::unique_ptr<MappedFile[]> tileCache; 
        
        void prepareTexture();
        
        Priv(Canvas c) : canvas(c) { texture.gen(); $.prepareTexture(); }
    };
    MEGA_PRIV_DTOR(TileManager)
    
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
        
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_SRGB8_ALPHA8, 
                     TEXTURE_SIZE, TEXTURE_SIZE, $.canvas.layers().size(),
                     0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        MEGA_ASSERT_GL_NO_ERROR;
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
    
    std::size_t TileManager::textureSize()
    {
        return TEXTURE_SIZE;
    }
    
    void TileManager::require(Rect r)
    {
        
    }
    
    void TileManager::centerHint(Vec center)
    {
        
    }
    
    void TileManager::viewportHint(Vec viewport, double zoom)
    {
        
    }
    
    bool TileManager::isTileReady(std::size_t tile)
    {
        return false;
    }
}