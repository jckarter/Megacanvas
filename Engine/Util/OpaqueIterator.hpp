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

    template<typename OpaqueT, typename T, T Deref(OpaqueT *, std::size_t)>
    struct OpaqueIterator {
        OpaqueT *ptr;
        std::size_t isize;

        typedef std::ptrdiff_t difference_type;
        typedef T value_type;
        typedef value_type *pointer;
        typedef value_type &reference;
        typedef std::random_access_iterator_tag iterator_category;

        OpaqueIterator() = default;
        OpaqueIterator(OpaqueT *ptr, std::size_t size)
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

    template<typename OpaqueT, typename T, T Deref(OpaqueT *, std::size_t)>
    struct OpaqueArrayRef {
        OpaqueT *data;
        std::size_t isize, length;

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
        // OpaqueArrayRef(U *, std::size_t)
        OpaqueArrayRef(OpaqueT *data, std::size_t isize, std::size_t length)
        : data(data), isize(isize), length(length)
        {}

        typedef OpaqueIterator<OpaqueT, T, Deref> iterator;
        typedef std::size_t size_type;

        iterator begin() const { return iterator(data, isize); }
        iterator end() const { return iterator(data + isize*length, isize); }
        bool empty() const { return length == 0; }
        std::size_t size() const { return length; }

        T front() const {
            assert(!empty());
            return Deref(data, isize);
        }

        T back() const {
            assert(!empty());
            return Deref(data + isize*(length-1), isize);
        }

        T operator[](std::size_t i) const {
            assert(i < length);
            return Deref(data + isize*i, isize);
        }

        bool equals(OpaqueArrayRef other) const {
            if (isize != other.isize || length != other.length)
                return false;
            if (data == other.data)
                return true;
            for (std::size_t i = 0; i < length; ++i)
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
        Array2DRef(T const *begin, std::size_t width, std::size_t height)
        : super(begin, width, height)
        {}
        
        Array2DRef(llvm::ArrayRef<T> array, std::size_t span)
        : super(array.data(), span, array.size() / span)
        {}

        Array2DRef(std::vector<T> const &vec, std::size_t span)
        : super(vec.data(), span, vec.size() / span)
        {}
    };
    
    template<typename T>
    llvm::MutableArrayRef<T> makeMutableArrayRef(T *begin, size_t size) {
        return {begin, size};
    }

    template<typename T>
    struct MutableArray2DRef : OpaqueArrayRef<T, llvm::MutableArrayRef<T>, makeMutableArrayRef<T>> {
        typedef OpaqueArrayRef<T, llvm::MutableArrayRef<T>, makeMutableArrayRef<T>> super;
        
        MutableArray2DRef() : super(nullptr, 1, 0) {}
        MutableArray2DRef(T *begin, std::size_t width, std::size_t height)
        : super(begin, width, height)
        {}
        
        MutableArray2DRef(llvm::MutableArrayRef<T> array, std::size_t span)
        : super(array.data(), span, array.size() / span)
        {}
        
        MutableArray2DRef(std::vector<T> &vec, std::size_t span)
        : super(vec.data(), span, vec.size() / span)
        {}
    };
}

#endif
