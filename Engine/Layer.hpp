//
//  Layer.hpp
//  Megacanvas
//
//  Created by Joe Groff on 6/21/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#ifndef Megacanvas_Layer_hpp
#define Megacanvas_Layer_hpp

#include <memory>
#include "Engine/Util/Priv.hpp"
#include "Engine/Vec.hpp"

namespace Mega {
    struct Layer : HasPriv<Layer> {
        MEGA_PRIV_CTORS(Layer)
        
        Vec parallax();
        void parallax(Vec x);
        
        int priority();
        void priority(int x);
    };
}

#endif
