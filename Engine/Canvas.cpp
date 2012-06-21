//
//  megacanvas.cpp
//  Megacanvas
//
//  Created by Joe Groff on 6/20/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#include "Engine/Canvas.hpp"
#include <vector>

namespace Mega {
    struct Canvas::priv {
        std::vector<Layer> layers;
    };
    
    Canvas::Canvas()
    : that(new priv()) {
        
    }
    
    Canvas::~Canvas()
    {
        
    }
    
    ArrayRef<Layer> Canvas::layers()
    {
        return makeArrayRef(that->layers);
    }
}