//
//  Tile.hpp
//  Megacanvas
//
//  Created by Joe Groff on 6/21/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#ifndef Megacanvas_Tile_hpp
#define Megacanvas_Tile_hpp

#include "Engine/Util/Priv.hpp"
#include <cstdint>

namespace Mega {
    struct Tile : HasPriv<Tile> {
        MEGA_PRIV_CTORS(Tile)
        
        ArrayRef<std::uint8_t> pixels();
        size_t size();
    };
}

#endif
