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
#include <utility>
#include "Engine/Util/Priv.hpp"
#include "Engine/Vec.hpp"

namespace Mega {
    struct Layer : HasPriv<Layer> {
        using tile_t = std::uint16_t;
        
        MEGA_PRIV_CTORS(Layer)

        Vec parallax();
        void parallax(Vec x);

        Vec origin();
        
        struct SegmentRef {        
            llvm::ArrayRef<tile_t> tiles;
            std::size_t offset;
        };
        SegmentRef segment(std::size_t segmentSize, std::ptrdiff_t x, std::ptrdiff_t y);
    };
}

#endif
