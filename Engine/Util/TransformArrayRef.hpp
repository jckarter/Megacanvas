//
//  TransformArrayRef.hpp
//  Megacanvas
//
//  Created by Joe Groff on 6/21/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#ifndef Megacanvas_TransformArrayRef_hpp
#define Megacanvas_TransformArrayRef_hpp

#include "Engine/Util/ArrayRef.hpp"
#include <iterator>

namespace Mega {
    template<typename T, typename U, U Function(T &)>
    struct TransformPointer {
        T *ptr;
        
        typedef std::ptrdiff_t difference_type;
        typedef U value_type;
        typedef U *pointer;
        typedef U &reference;
        typedef std::random_access_iterator_tag iterator_category;

#define MEGA_COMPARE_OPERATOR(op) \
    bool operator op(TransformPointer const &p) const { return ptr op p.ptr; }

        MEGA_COMPARE_OPERATOR(==)
        MEGA_COMPARE_OPERATOR(!=)
        MEGA_COMPARE_OPERATOR(<)
        MEGA_COMPARE_OPERATOR(<=)
        MEGA_COMPARE_OPERATOR(>=)
        MEGA_COMPARE_OPERATOR(>)
#undef MEGA_COMPARE_OPERATOR
        
#define MEGA_INCDEC_OPERATOR(op) \
        TransformPointer &operator op() { op ptr; return *this; } \
        TransformPointer operator op(int) { \
            TransformPointer r{ptr}; \
            op ptr; \
            return r; \
        }
        
        MEGA_INCDEC_OPERATOR(--)
        MEGA_INCDEC_OPERATOR(++)
#undef MEGA_INCDEC_OPERATOR
        
#define MEGA_MATH_OPERATOR(op) \
    TransformPointer operator op(std::ptrdiff_t dist) const { \
        return TransformPointer<T,U,Function>{ptr op dist}; \
    }

        MEGA_MATH_OPERATOR(+)
        MEGA_MATH_OPERATOR(-)
#undef MEGA_MATH_OPERATOR

#define MEGA_COMPOUND_OPERATOR(op) \
    TransformPointer &operator op(std::ptrdiff_t dist) { \
        ptr op dist; \
        return *this; \
    }
        
        MEGA_COMPOUND_OPERATOR(+=)
        MEGA_COMPOUND_OPERATOR(-=)
#undef MEGA_COMPOUND_OPERATOR
        
        std::ptrdiff_t operator-(TransformPointer const &p) const { return ptr - p.ptr; }

        U operator*()  const { return Function(*ptr); }
        U operator->() const { return Function(*ptr); }
        U operator[](std::ptrdiff_t i) const { return Function(ptr[i]); }
    };
}

#endif
