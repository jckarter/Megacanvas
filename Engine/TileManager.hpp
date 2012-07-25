//
//  TileManager.hpp
//  Megacanvas
//
//  Created by Joe Groff on 7/24/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#ifndef Megacanvas_TileManager_hpp
#define Megacanvas_TileManager_hpp

#include "Engine/Util/Priv.hpp"
#include "Engine/Vec.hpp"

namespace Mega {
    struct Canvas;
    
    struct TileManager : HasPriv<TileManager> {
        MEGA_PRIV_CTORS(TileManager)
        
        static Owner<TileManager> create(Canvas c);
        
        void bindState();
        GLuint texture();
        std::size_t textureSize();
        
        void require(Rect r);
        
        void centerHint(Vec center);
        void viewportHint(Vec viewport, double zoom);
        
        bool isTileReady(std::size_t tile);
    };
}

#endif
