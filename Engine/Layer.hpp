//
//  Layer.hpp
//  Megacanvas
//
//  Created by Joe Groff on 6/21/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#ifndef Megacanvas_Layer_hpp
#define Megacanvas_Layer_hpp

#include <functional>
#include <memory>
#include "Engine/Util/Priv.hpp"
#include "Engine/Vec.hpp"

namespace Mega {
    struct Layer : HasPriv<Layer> {
        using tile_t = std::uint16_t;
        
        MEGA_PRIV_CTORS(Layer)

        Vec parallax();
        void parallax(Vec x);

        Vec origin();
        
        void getSegment(std::ptrdiff_t x, std::ptrdiff_t y,
                        tile_t *outBuffer, std::size_t segmentSize);
    };
}

#endif
