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
    struct Canvas::priv {
        std::vector<Layer> layers;
    };
    
    Canvas::Canvas()
    : that(nullptr) {
        
    }
    
    Canvas::~Canvas()
    {
        
    }
    
    ArrayRef<Layer> Canvas::layers()
    {
        if (that)
            return that->layers;
        else
            return ArrayRef<Layer>();
    }
}