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
    typedef std::pair<double, double> Vec;
    
    inline Vec makeVec(double x, double y) { return std::make_pair(x, y); }
}

#endif
