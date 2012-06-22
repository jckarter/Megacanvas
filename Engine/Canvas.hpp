//
//  Canvas.hpp
//  Megacanvas
//
//  Created by Joe Groff on 6/20/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#ifndef Megacanvas_megacanvas_h
#define Megacanvas_megacanvas_h

#include "Engine/Util/ArrayRef.hpp"
#include "Engine/Util/StringRef.hpp"
#include "Engine/Util/Priv.hpp"
#include <memory>

namespace Mega {
    typedef std::pair<double, double> Point;
    struct Layer;
    struct Tile;
    
    struct Canvas : HasPriv<Canvas> {        
        MEGA_PRIV_CTORS(Canvas)
        
        static PrivOwner<Canvas> create();
        static PrivOwner<Canvas> load(StringRef path);
        
        ArrayRef<Priv<Layer>> layers();
        ArrayRef<Priv<Tile>> tiles();
        
        Layer layer(size_t i);
        Tile tile(size_t i);
    };
}

#endif
