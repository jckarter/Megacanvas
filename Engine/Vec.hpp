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
    struct BoolVec {
        bool x, y;
        
        BoolVec() = default;
        
        constexpr bool both() const { return x && y; }
        constexpr bool either() const { return x || y; }
        
        constexpr BoolVec operator&(BoolVec o) const { return BoolVec{x && o.x, y && o.y}; }
        constexpr BoolVec operator|(BoolVec o) const { return BoolVec{x || o.x, y || o.y}; }
    };
    
    inline constexpr bool both(BoolVec b) { return b.x && b.y; }
    inline constexpr bool either(BoolVec b) { return b.x || b.y; }
    
    struct Vec {
        double x, y;
        Vec() = default;

        constexpr bool operator==(Vec o) const { return x == o.x && y == o.y; }
        constexpr bool operator!=(Vec o) const { return x != o.x || y != o.y; }
        
#define _MEGA_VEC_MATH_OP(op) \
    Vec operator op(Vec o) const { return Vec{x op o.x, y op o.y}; } \
    Vec operator op(double o) const { return Vec{x op o, y op o}; } \
    Vec &operator op##=(Vec o) { x op##= o.x; y op##= o.y; return *this; } \
    friend Vec operator op(double o, Vec v) { return Vec{o op v.x, o op v.y}; }
        _MEGA_VEC_MATH_OP(+)
        _MEGA_VEC_MATH_OP(-)
        _MEGA_VEC_MATH_OP(*)
        _MEGA_VEC_MATH_OP(/)
#undef _MEGA_VEC_MATH_OP
        
#define _MEGA_VEC_MATH_METHOD(op) \
    Vec op() const { return Vec{std::op(x), std::op(y)}; }
        _MEGA_VEC_MATH_METHOD(floor)
        _MEGA_VEC_MATH_METHOD(ceil)
        _MEGA_VEC_MATH_METHOD(round)
        _MEGA_VEC_MATH_METHOD(trunc)
#undef _MEGA_VEC_MATH_METHOD

#define _MEGA_VEC_COMPARE_OP(op) \
    BoolVec operator op(Vec o) const { return BoolVec{x op o.x, y op o.y}; } \
    BoolVec operator op(double o) const { return BoolVec{x op o, y op o}; } \
    friend BoolVec operator op(double o, Vec v) { return BoolVec{o op v.x, o op v.y}; }
        _MEGA_VEC_COMPARE_OP(<=)
        _MEGA_VEC_COMPARE_OP(<)
        _MEGA_VEC_COMPARE_OP(>)
        _MEGA_VEC_COMPARE_OP(>=)
#undef _MEGA_VEC_COMPARE_OP
    };
    
    struct Rect {
        Vec lo, hi;
        
        Rect() = default;
        Rect(Vec lo, Vec hi) : lo(lo), hi(hi) {}
        constexpr Rect(double lox, double loy, double hix, double hiy)
        : lo{lox, loy}, hi{hix, hiy} {}
        
        bool contains(Vec v) const { return both(lo <= v & v < hi); }
        
        bool contains(Rect r) const { return contains(r.lo) && contains(r.hi); };
        
        Rect intersect(Rect r) const {
            return {
                std::max(lo.x, r.lo.x), std::max(lo.y, r.lo.y),
                std::min(hi.x, r.hi.x), std::min(hi.y, r.hi.y)
            };
        }
    };
}

#endif
