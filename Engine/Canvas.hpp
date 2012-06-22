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
#include <memory>

namespace Mega {
    class Layer;
    
    class Canvas {
        struct priv;
        std::unique_ptr<priv> that;
    public:
        Canvas();
        ~Canvas();
        
        ArrayRef<Layer> layers();
    };
}

#endif
