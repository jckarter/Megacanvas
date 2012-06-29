//
//  TransformArrayRef.hpp
//  Megacanvas
//
//  Created by Joe Groff on 6/21/12.
//  Copyright (c) 2012 Durian Software. All rights reserved.
//

#ifndef Megacanvas_TransformArrayRef_hpp
#define Megacanvas_TransformArrayRef_hpp

#include <llvm/ADT/ArrayRef.h>
#include <iterator>

namespace Mega {
    template<typename T>
    struct DerefWrapper {
        T that;
        T *operator->() { return &that; }
        T const *operator->() const { return &that; }
        
        DerefWrapper() = default;
        DerefWrapper(T const &that) : that(that) {}
        DerefWrapper(T &&that) : that(that) {}
    };
    
    template<typename OpaqueT, typename T, T Deref(OpaqueT *, size_t)>
    struct OpaqueIterator {
        OpaqueT *ptr;
        size_t isize;
        
        typedef std::ptrdiff_t difference_type;
        typedef T value_type;
        typedef value_type *pointer;
        typedef value_type &reference;
        typedef std::random_access_iterator_tag iterator_category;
        
        OpaqueIterator() = default;
        OpaqueIterator(OpaqueT *ptr, size_t size)
        : ptr(ptr), isize(size)
        {}
        
#define _MEGA_COMPARE_OPERATOR(op) \
    bool operator op(OpaqueIterator const &p) const { return ptr op p.ptr; }
        
        _MEGA_COMPARE_OPERATOR(==)
        _MEGA_COMPARE_OPERATOR(!=)
        _MEGA_COMPARE_OPERATOR(<)
        _MEGA_COMPARE_OPERATOR(<=)
        _MEGA_COMPARE_OPERATOR(>=)
        _MEGA_COMPARE_OPERATOR(>)
#undef _MEGA_COMPARE_OPERATOR
        
#define _MEGA_INCDEC_OPERATOR(op, arop) \
    OpaqueIterator &operator op() { ptr arop isize; return *this; } \
    OpaqueIterator operator op(int) { \
        OpaqueIterator r = *this; \
        ptr arop isize; \
        return r; \
    }
        
        _MEGA_INCDEC_OPERATOR(--, -=)
        _MEGA_INCDEC_OPERATOR(++, +=)
#undef _MEGA_INCDEC_OPERATOR
        
#define _MEGA_MATH_OPERATOR(op) \
    OpaqueIterator operator op(std::ptrdiff_t dist) const { \
        return OpaqueIterator(ptr op (dist*isize), isize); \
    }
        
        _MEGA_MATH_OPERATOR(+)
        _MEGA_MATH_OPERATOR(-)
#undef _MEGA_MATH_OPERATOR
        
#define _MEGA_COMPOUND_OPERATOR(op) \
    OpaqueIterator &operator op(std::ptrdiff_t dist) { \
        ptr op (dist*isize); \
        return *this; \
    }
        
        _MEGA_COMPOUND_OPERATOR(+=)
        _MEGA_COMPOUND_OPERATOR(-=)
#undef _MEGA_COMPOUND_OPERATOR
        
        std::ptrdiff_t operator-(OpaqueIterator const &p) const { return ptr - p.ptr; }
        
        T operator*()  const { return Deref(ptr, isize); }
        DerefWrapper<T> operator->() const { return DerefWrapper<T>(Deref(ptr, isize)); }
        T operator[](std::ptrdiff_t i) const {
            return Deref(ptr + i*isize, isize);
        }
    };
    
    template<typename OpaqueT, typename T, T Deref(OpaqueT *, size_t)>
    struct OpaqueArrayRef {
        OpaqueT *data;
        size_t isize, length;
        
        OpaqueArrayRef() : data(nullptr), isize(0), length(0) {}
        OpaqueArrayRef(const OpaqueArrayRef &) = default;
        
        template<typename U>
        OpaqueArrayRef(U &oneElt)
        : data(reinterpret_cast<OpaqueT*>(&oneElt)), isize(sizeof(U)), length(1)
        {}
        
        template<typename U>
        OpaqueArrayRef(std::vector<U> &vec)
        : data(reinterpret_cast<OpaqueT*>(&vec[0])), isize(sizeof(U)), length(vec.size())
        {}
        // OpaqueArrayRef(U (&)[N])
        // OpaqueArrayRef(SmallVectorImpl)
        // OpaqueArrayRef(U *, U *)
        // OpaqueArrayRef(U *, size_t)
        OpaqueArrayRef(OpaqueT *data, size_t isize, size_t length)
        : data(data), isize(isize), length(length)
        {}
        
        typedef OpaqueIterator<OpaqueT, T, Deref> iterator;
        typedef size_t size_type;
        
        iterator begin() const { return iterator(data, isize); }
        iterator end() const { return iterator(data + isize*length, isize); }
        bool empty() const { return length == 0; }
        size_t size() const { return length; }
        
        T front() const {
            assert(!empty());
            return Deref(data, isize);
        }
        
        T back() const {
            assert(!empty());
            return Deref(data + isize*(length-1), isize);
        }
        
        T operator[](size_t i) const {
            assert(i < length);
            return Deref(data + isize*i, isize);
        }
        
        bool equals(OpaqueArrayRef other) const {
            if (this->isize != other.isize || this->length != other.length)
                return false;
            if (this->data == other.data)
                return true;
            for (size_t i = 0; i < this->length; ++i)
                if ((*this)[i] != other[i])
                    return false;
            return true;
        }
        // slice
    };
    
    template<typename T>
    struct Array2DRef : OpaqueArrayRef<T const, llvm::ArrayRef<T>, llvm::makeArrayRef<T>> {
        typedef OpaqueArrayRef<T const, llvm::ArrayRef<T>, llvm::makeArrayRef<T>> super;
        
        Array2DRef() : super(nullptr, 1, 0) {}
        Array2DRef(llvm::ArrayRef<T> array, size_t span)
        : super(array.data(), span, array.size() / span)
        {}
        
        Array2DRef(std::vector<T> const &vec, size_t span)
        : super(vec.data(), span, vec.size() / span)
        {}
    };
    
}

#endif
