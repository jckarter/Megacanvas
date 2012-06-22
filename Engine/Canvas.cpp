//
//  Canvas.cpp
//  Megacanvas
//
//  Created by Joe Groff on 6/20/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#include "Engine/Canvas.hpp"
#include "Engine/Layer.hpp"
#include <vector>

namespace Mega {
    template<>
    struct Priv<Canvas> {
        std::vector<Priv<Layer>> layers;
    };
    
    MEGA_PRIV_DTOR(Canvas)
    
    template<>
    struct Priv<Layer> {
        
    };
    
    MEGA_PRIV_DTOR(Layer)
    
    PrivOwner<Canvas> Canvas::create()
    {
        return PrivOwner<Canvas>();
    }
    
    ArrayRef<Priv<Layer>> Canvas::layers()
    {
        return that->layers;
    }
}