//
//  Vec.hpp
//  Megacanvas
//
//  Created by Joe Groff on 6/22/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#ifndef Megacanvas_Vec_hpp
#define Megacanvas_Vec_hpp

namespace Mega {
    struct Vec {
        double x, y;
        constexpr Vec() = default;
        constexpr Vec(double x, double y) : x(x), y(y) {}
        
        constexpr bool operator==(Vec o) { return x == o.x && y == o.y; }
        constexpr bool operator!=(Vec o) { return x != o.x || y != o.y; }
    };
}

#endif
