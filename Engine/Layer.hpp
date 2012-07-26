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
    size_t swizzle(std::size_t x, std::size_t y);
    
    struct Layer : HasPriv<Layer> {
        using tile_t = std::uint16_t;
        
        MEGA_PRIV_CTORS(Layer)

        Vec parallax();
        void parallax(Vec x);

        Vec origin();
        
        struct SegmentRef {        
            llvm::ArrayRef<tile_t> tiles;
            std::size_t offset;
            
            tile_t operator[](size_t i) const
            {
                if (i >= offset) {
                    i -= offset;
                    if (i < tiles.size())
                        return tiles[i];
                }
                return 0;
            }
            
            tile_t at(size_t x, size_t y) const { return (*this)[swizzle(x, y)]; }
        };
        SegmentRef segment(std::size_t segmentSize, std::ptrdiff_t x, std::ptrdiff_t y);
    };
}

#endif
