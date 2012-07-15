//
//  Vec.hpp
//  Megacanvas
//
//  Created by Joe Groff on 6/22/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#ifndef Megacanvas_Vec_hpp
#define Megacanvas_Vec_hpp

#include <cmath>

namespace Mega {
    struct Vec {
        double x, y;
        constexpr Vec() = default;
        constexpr Vec(double x, double y) : x(x), y(y) {}

        constexpr bool operator==(Vec o) const { return x == o.x && y == o.y; }
        constexpr bool operator!=(Vec o) const { return x != o.x || y != o.y; }
        
#define _MEGA_VEC_MATH_OP(op) \
    Vec operator op(Vec o) const { return Vec(x op o.x, y op o.y); } \
    Vec operator op(double o) const { return Vec(x op o, y op o); } \
    Vec &operator op##=(Vec o) { x op##= o.x; y op##= o.y; return *this; } \
    friend Vec operator op(double o, Vec v) { return Vec(o op v.x, o op v.y); }
        _MEGA_VEC_MATH_OP(+)
        _MEGA_VEC_MATH_OP(-)
        _MEGA_VEC_MATH_OP(*)
        _MEGA_VEC_MATH_OP(/)
#undef _MEGA_VEC_MATH_OP
        
#define _MEGA_VEC_MATH_METHOD(op) \
    Vec op() const { return Vec(std::op(x), std::op(y)); }
        _MEGA_VEC_MATH_METHOD(floor)
        _MEGA_VEC_MATH_METHOD(ceil)
        _MEGA_VEC_MATH_METHOD(round)
        _MEGA_VEC_MATH_METHOD(trunc)
#undef _MEGA_VEC_MATH_METHOD
    };
}

#endif
